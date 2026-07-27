#include "soul/cache/disk_cache.h"
#include "soul/core/error.h"
#include "soul/logging/log_macros.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace sc {
namespace cache {

namespace {

/**
 * @brief 读取 TTL 文件中的过期时间戳(毫秒)
 * @return 过期时间;文件不存在或损坏时返回 -1(视为无 TTL)
 */
qint64 readExpiryMSecs(const QString& ttlPath) {
    QFile ttlFile(ttlPath);
    if (!ttlFile.exists() || !ttlFile.open(QIODevice::ReadOnly)) {
        return -1;
    }
    bool ok = false;
    qint64 expiry = QString::fromUtf8(ttlFile.readAll()).trimmed().toLongLong(&ok);
    return ok ? expiry : -1;
}

/**
 * @brief 写入过期时间戳到 TTL 文件(毫秒)
 */
bool writeExpiryMSecs(const QString& ttlPath, qint64 expiryMSecs) {
    QFile ttlFile(ttlPath);
    if (!ttlFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    ttlFile.write(QString::number(expiryMSecs).toUtf8());
    return true;
}

/**
 * @brief 查询数据文件大小(字节)
 * @return 文件大小;文件不存在时返回 0
 */
qint64 dataSizeOnDisk(const QString& dataPath) {
    QFileInfo info(dataPath);
    return info.exists() ? info.size() : 0;
}

} // namespace

DiskCache::DiskCache(Config config)
    : m_config(std::move(config)) {
    ensureCacheDir();
    // 启动时扫描磁盘恢复 totalBytes 和 LRU 索引,使 maxBytes 限制在重启后仍生效
    std::lock_guard<std::mutex> lock(m_mutex);
    recoverFromDiskUnlocked();
}

QString DiskCache::computeHash(const std::string& key) const {
    const QByteArray raw = QCryptographicHash::hash(
        QByteArray::fromRawData(key.data(), static_cast<int>(key.size())),
        QCryptographicHash::Sha256);
    return QString::fromUtf8(raw.toHex());
}

QString DiskCache::keyToDataPath(const std::string& key) const {
    const QString hash = computeHash(key);
    // 前 2 字符作为子目录,分散文件
    const QString subDir = hash.left(2);
    const QString fileName = hash.mid(2);
    return QDir(m_config.cacheDir).filePath(subDir + QLatin1Char('/') + fileName + QStringLiteral(".dat"));
}

QString DiskCache::keyToTtlPath(const std::string& key) const {
    return keyToDataPath(key) + QStringLiteral(".ttl");
}

bool DiskCache::isExpired(const std::string& key) const {
    const QString ttlPath = keyToTtlPath(key);
    // TTL 文件不存在 → 未设置 TTL,永不过期
    if (!QFile::exists(ttlPath)) {
        return false;
    }
    const qint64 expiry = readExpiryMSecs(ttlPath);
    if (expiry < 0) {
        // TTL 文件存在但损坏 → 保守策略,视为过期(宁可 miss 不可复活过期数据)
        return true;
    }
    return QDateTime::currentDateTime().toMSecsSinceEpoch() > expiry;
}

void DiskCache::ensureCacheDir() const {
    if (m_config.cacheDir.isEmpty()) {
        return;
    }
    QDir dir(m_config.cacheDir);
    if (!dir.exists()) {
        dir.mkpath(m_config.cacheDir);
    }
}

void DiskCache::recoverFromDiskUnlocked() {
    // 扫描磁盘已存在的 .dat 文件,恢复 totalBytes 和 LRU 索引
    // 调用者必须持有 m_mutex
    // LRU 索引使用文件路径作为 key,因此可以直接从磁盘重建
    QDir dir(m_config.cacheDir);
    if (!dir.exists()) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const qint64 nowWallMs = QDateTime::currentDateTime().toMSecsSinceEpoch();

    const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& sub : subDirs) {
        QDir subDir(dir.filePath(sub));
        const QStringList dataFiles = subDir.entryList({QStringLiteral("*.dat")}, QDir::Files);
        for (const QString& f : dataFiles) {
            const QString filePath = subDir.filePath(f);
            QFileInfo info(filePath);
            if (!info.exists()) {
                continue;
            }
            m_stats.totalBytes += static_cast<std::size_t>(info.size());

            // 重建 LRU 索引:将文件 mtime(wall clock)转换为 steady_clock 时间点
            // 转换公式:steady_time = now - (nowWallMs - fileMtimeMs)
            // 这样旧文件的 steady_time 较小,淘汰时优先被选中
            const qint64 fileMtimeMs = info.lastModified().toMSecsSinceEpoch();
            const auto fileAgeMs = std::chrono::milliseconds(
                nowWallMs > fileMtimeMs ? (nowWallMs - fileMtimeMs) : 0);
            m_lruIndex[filePath.toStdString()] = now - fileAgeMs;
        }
    }
}

bool DiskCache::evictToFitUnlocked(std::size_t incomingSize) {
    // 淘汰最久未访问的条目,直到能容纳 incomingSize
    // 调用者必须持有 m_mutex
    if (m_config.maxBytes == 0) {
        return true;  // 无限制
    }
    if (incomingSize > m_config.maxBytes) {
        return false;  // 单条超限,无法容纳
    }
    while (m_stats.totalBytes + incomingSize > m_config.maxBytes && !m_lruIndex.empty()) {
        evictLruUnlocked();
    }
    return m_stats.totalBytes + incomingSize <= m_config.maxBytes;
}

void DiskCache::evictLruUnlocked() {
    // 淘汰最久未访问的条目(基于 steady_clock 时间戳)
    // 调用者必须持有 m_mutex
    // LRU 索引键为文件路径,直接通过路径删除文件
    if (m_lruIndex.empty()) {
        return;
    }
    // 找到时间戳最小的(最久未访问)
    auto oldest = std::min_element(
        m_lruIndex.begin(), m_lruIndex.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });
    if (oldest == m_lruIndex.end()) {
        return;
    }
    const std::string pathStr = oldest->first;
    const QString dataPath = QString::fromStdString(pathStr);
    const QString ttlPath = dataPath + QStringLiteral(".ttl");

    const qint64 oldSize = dataSizeOnDisk(dataPath);
    const bool dataRemoved = QFile::remove(dataPath);
    QFile::remove(ttlPath);

    if (dataRemoved && oldSize > 0 && m_stats.totalBytes >= static_cast<std::size_t>(oldSize)) {
        m_stats.totalBytes -= static_cast<std::size_t>(oldSize);
        m_stats.evictionCount++;
    }
    m_lruIndex.erase(pathStr);
}

void DiskCache::touchUnlocked(const std::string& key) {
    // 更新键的最后访问时间
    // 调用者必须持有 m_mutex
    // LRU 索引键为文件路径,而非原始 key
    const QString dataPath = keyToDataPath(key);
    m_lruIndex[dataPath.toStdString()] = std::chrono::steady_clock::now();
}

bool DiskCache::writeAtomically(const QString& targetPath, const QByteArray& data) {
    // 原子写入:先写临时文件,成功后 rename 到目标路径
    // 失败时临时文件被清理,目标文件不受影响
    const QString tmpPath = targetPath + QStringLiteral(".tmp");

    QFile tmpFile(tmpPath);
    if (!tmpFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        SC_ERROR("DiskCache: cannot open temp file for write: " + tmpPath.toStdString());
        return false;
    }

    const qint64 written = tmpFile.write(data);
    if (written != data.size()) {
        SC_ERROR("DiskCache: short write to temp file");
        tmpFile.close();
        QFile::remove(tmpPath);
        return false;
    }

    tmpFile.flush();
    tmpFile.close();

    // 检查 close 是否成功(flush 失败会反映在 close)
    if (tmpFile.error() != QFile::NoError) {
        SC_ERROR("DiskCache: temp file close failed: " + tmpFile.errorString().toStdString());
        QFile::remove(tmpPath);
        return false;
    }

    // rename 原子替换(同分区)
    if (!QFile::rename(tmpPath, targetPath)) {
        SC_ERROR("DiskCache: rename failed: " + tmpPath.toStdString() +
                 " -> " + targetPath.toStdString());
        QFile::remove(tmpPath);
        return false;
    }

    return true;
}

Result<std::optional<std::string>> DiskCache::get(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const QString dataPath = keyToDataPath(key);
    if (!QFile::exists(dataPath)) {
        m_stats.missCount++;
        return std::optional<std::string>{};
    }

    if (isExpired(key)) {
        // 过期:清理文件并修正 totalBytes 和 LRU 索引
        const qint64 oldSize = dataSizeOnDisk(dataPath);
        QFile::remove(dataPath);
        QFile::remove(keyToTtlPath(key));
        if (oldSize > 0 && m_stats.totalBytes >= static_cast<std::size_t>(oldSize)) {
            m_stats.totalBytes -= static_cast<std::size_t>(oldSize);
        }
        m_lruIndex.erase(dataPath.toStdString());
        m_stats.missCount++;
        return std::optional<std::string>{};
    }

    QFile file(dataPath);
    if (!file.open(QIODevice::ReadOnly)) {
        SC_ERROR("DiskCache: failed to open data file: " + dataPath.toStdString());
        return Error(ErrorCode::InternalError, "DiskCache: cannot open data file");
    }

    const QByteArray data = file.readAll();
    m_stats.hitCount++;
    touchUnlocked(key);  // 更新 LRU 索引(命中时刷新访问时间)
    return std::optional<std::string>{std::string(data.constData(), static_cast<std::size_t>(data.size()))};
}

Result<void> DiskCache::put(const std::string& key, const std::string& value,
                            std::optional<std::chrono::milliseconds> ttl) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const QString dataPath = keyToDataPath(key);
    const QString dir = QFileInfo(dataPath).absolutePath();

    if (!QDir().mkpath(dir)) {
        SC_ERROR("DiskCache: cannot create directory: " + dir.toStdString());
        return Error(ErrorCode::InternalError, "DiskCache: cannot create directory");
    }

    // 先检查 oldSize(不修改 totalBytes),用于后续空间计算
    const qint64 oldSize = dataSizeOnDisk(dataPath);

    // 淘汰最久未访问的条目,直到能容纳新数据(防止磁盘无限增长)
    // 注意:此处传入 (value.size() - oldSize) 表示净增量;若 oldSize >= value.size(),
    //       说明覆盖后空间足够,无需淘汰。但 evictToFitUnlocked 的语义是"为 incomingSize
    //       腾出空间",因此传入净增量更准确。为简化逻辑,这里传入 value.size(),
    //       并在淘汰后统一处理 oldSize 的扣减。
    if (!evictToFitUnlocked(value.size())) {
        SC_ERROR("DiskCache: cannot evict enough space for new entry (size=" +
                 std::to_string(value.size()) + ", maxBytes=" +
                 std::to_string(m_config.maxBytes) + ")");
        // 失败时不修改任何状态(totalBytes 未变),保持一致性
        return Error(ErrorCode::InternalError, "DiskCache: cache full, cannot evict enough space");
    }

    // 原子写入:先写临时文件,成功后 rename(防止崩溃时数据损坏)
    const QByteArray data(value.data(), static_cast<int>(value.size()));
    if (!writeAtomically(dataPath, data)) {
        // 失败时不修改任何状态(totalBytes 未变,LRU 未 touch),保持一致性
        return Error(ErrorCode::InternalError, "DiskCache: atomic write failed");
    }

    // 写入成功后,统一修正 totalBytes:先减旧值(若有),再加新值
    if (oldSize > 0 && m_stats.totalBytes >= static_cast<std::size_t>(oldSize)) {
        m_stats.totalBytes -= static_cast<std::size_t>(oldSize);
    }
    m_stats.totalBytes += static_cast<std::size_t>(value.size());
    touchUnlocked(key);  // 更新 LRU 索引

    // 写入 TTL(如果有)
    std::chrono::milliseconds effectiveTtl = ttl.value_or(
        m_config.defaultTtl.value_or(std::chrono::milliseconds::zero()));
    if (effectiveTtl.count() > 0) {
        const qint64 expiry = QDateTime::currentDateTime()
                                  .addMSecs(effectiveTtl.count())
                                  .toMSecsSinceEpoch();
        if (!writeExpiryMSecs(keyToTtlPath(key), expiry)) {
            SC_WARN("DiskCache: cannot write TTL file for key hash");
            // TTL 写入失败不阻断数据写入
        }
    } else {
        // 无 TTL:移除可能存在的旧 TTL 文件
        QFile::remove(keyToTtlPath(key));
    }

    return {};
}

Result<void> DiskCache::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const QString dataPath = keyToDataPath(key);
    const QString ttlPath = keyToTtlPath(key);

    bool removed = false;
    if (QFile::exists(dataPath)) {
        const qint64 oldSize = dataSizeOnDisk(dataPath);
        removed = QFile::remove(dataPath);
        if (removed && oldSize > 0 && m_stats.totalBytes >= static_cast<std::size_t>(oldSize)) {
            m_stats.totalBytes -= static_cast<std::size_t>(oldSize);
        }
    }
    if (QFile::exists(ttlPath)) {
        QFile::remove(ttlPath);
    }

    // 同步清理 LRU 索引(无论文件是否存在,索引可能残留)
    m_lruIndex.erase(dataPath.toStdString());

    if (!removed) {
        return Error(ErrorCode::NotFound, "DiskCache: key not found");
    }
    return {};
}

Result<bool> DiskCache::contains(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    const QString dataPath = keyToDataPath(key);
    if (!QFile::exists(dataPath)) {
        return false;
    }
    if (isExpired(key)) {
        const qint64 oldSize = dataSizeOnDisk(dataPath);
        QFile::remove(dataPath);
        QFile::remove(keyToTtlPath(key));
        if (oldSize > 0 && m_stats.totalBytes >= static_cast<std::size_t>(oldSize)) {
            m_stats.totalBytes -= static_cast<std::size_t>(oldSize);
        }
        m_lruIndex.erase(dataPath.toStdString());
        return false;
    }
    touchUnlocked(key);  // 命中时刷新 LRU 访问时间
    return true;
}

Result<void> DiskCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    QDir dir(m_config.cacheDir);
    if (!dir.exists()) {
        return {};
    }

    const QStringList entries = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& sub : entries) {
        QDir subDir(dir.filePath(sub));
        subDir.removeRecursively();
    }

    // 清理根目录下的残留文件
    const QStringList files = dir.entryList(QDir::Files);
    for (const QString& f : files) {
        dir.remove(f);
    }

    m_stats.totalBytes = 0;
    m_lruIndex.clear();  // 清空 LRU 索引
    return {};
}

Result<std::unordered_map<std::string, std::string>>
DiskCache::getMany(const std::vector<std::string>& keys) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::unordered_map<std::string, std::string> result;
    result.reserve(keys.size());

    for (const std::string& key : keys) {
        const QString dataPath = keyToDataPath(key);
        if (!QFile::exists(dataPath)) {
            m_stats.missCount++;
            continue;
        }
        if (isExpired(key)) {
            const qint64 oldSize = dataSizeOnDisk(dataPath);
            QFile::remove(dataPath);
            QFile::remove(keyToTtlPath(key));
            if (oldSize > 0 && m_stats.totalBytes >= static_cast<std::size_t>(oldSize)) {
                m_stats.totalBytes -= static_cast<std::size_t>(oldSize);
            }
            m_lruIndex.erase(dataPath.toStdString());
            m_stats.missCount++;
            continue;
        }

        QFile file(dataPath);
        if (!file.open(QIODevice::ReadOnly)) {
            m_stats.missCount++;
            continue;
        }
        const QByteArray data = file.readAll();
        m_stats.hitCount++;
        touchUnlocked(key);  // 命中时刷新 LRU 访问时间
        result.emplace(key, std::string(data.constData(), static_cast<std::size_t>(data.size())));
    }
    return result;
}

Result<void> DiskCache::putMany(const std::unordered_map<std::string, std::string>& entries,
                                std::optional<std::chrono::milliseconds> ttl) {
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [key, value] : entries) {
        const QString dataPath = keyToDataPath(key);
        const QString dir = QFileInfo(dataPath).absolutePath();

        if (!QDir().mkpath(dir)) {
            SC_ERROR("DiskCache: cannot create directory: " + dir.toStdString());
            return Error(ErrorCode::InternalError, "DiskCache: cannot create directory");
        }

        // 先检查 oldSize(不修改 totalBytes),用于后续空间计算
        const qint64 oldSize = dataSizeOnDisk(dataPath);

        // 淘汰最久未访问的条目,直到能容纳新数据
        if (!evictToFitUnlocked(value.size())) {
            SC_ERROR("DiskCache: cannot evict enough space in putMany (size=" +
                     std::to_string(value.size()) + ")");
            return Error(ErrorCode::InternalError, "DiskCache: cache full, cannot evict enough space");
        }

        // 原子写入
        const QByteArray data(value.data(), static_cast<int>(value.size()));
        if (!writeAtomically(dataPath, data)) {
            return Error(ErrorCode::InternalError, "DiskCache: atomic write failed in putMany");
        }

        // 写入成功后,统一修正 totalBytes:先减旧值(若有),再加新值
        if (oldSize > 0 && m_stats.totalBytes >= static_cast<std::size_t>(oldSize)) {
            m_stats.totalBytes -= static_cast<std::size_t>(oldSize);
        }
        m_stats.totalBytes += static_cast<std::size_t>(value.size());
        touchUnlocked(key);  // 更新 LRU 索引

        std::chrono::milliseconds effectiveTtl = ttl.value_or(
            m_config.defaultTtl.value_or(std::chrono::milliseconds::zero()));
        if (effectiveTtl.count() > 0) {
            const qint64 expiry = QDateTime::currentDateTime()
                                      .addMSecs(effectiveTtl.count())
                                      .toMSecsSinceEpoch();
            writeExpiryMSecs(keyToTtlPath(key), expiry);
        } else {
            QFile::remove(keyToTtlPath(key));
        }
    }
    return {};
}

std::size_t DiskCache::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::size_t count = 0;
    QDir dir(m_config.cacheDir);
    if (!dir.exists()) {
        return 0;
    }

    const QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString& sub : subDirs) {
        QDir subDir(dir.filePath(sub));
        const QStringList dataFiles = subDir.entryList({QStringLiteral("*.dat")}, QDir::Files);
        count += static_cast<std::size_t>(dataFiles.size());
    }
    return count;
}

CacheStats DiskCache::stats() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

} // namespace cache
} // namespace sc
