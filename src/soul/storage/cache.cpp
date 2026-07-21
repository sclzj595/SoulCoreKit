#include "soul/storage/cache.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"
#include <QFile>
#include <QDir>
#include <QCryptographicHash>

namespace sc {

DiskCache::DiskCache(const QString& cacheDir, size_t maxSize)
    : m_cacheDir(cacheDir), m_maxSize(maxSize) {
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(cacheDir);
    }
}

Result<void> DiskCache::put(const QString& key, const QByteArray& value) {
    return put(key, value, std::chrono::seconds(0));
}

Result<void> DiskCache::put(const QString& key, const QByteArray& value, std::chrono::seconds ttl) {
    std::lock_guard<std::mutex> lock(m_mutex);

    QString filePath = getFilePath(key);
    QString dir = QFileInfo(filePath).absolutePath();

    if (!QDir().mkpath(dir)) {
        SC_ERROR("Cannot create cache directory: " + dir.toStdString());
        return Error(ErrorCode::InternalError, "Cannot create cache directory: " + dir.toStdString());
    }

    auto existingIt = m_entrySizes.find(key);
    if (existingIt != m_entrySizes.end()) {
        m_currentSize -= existingIt->second;
        m_lruList.remove(key);
    }

    if (m_currentSize + value.size() > m_maxSize && m_maxSize > 0) {
        evict();
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        SC_ERROR("Cannot open cache file: " + filePath.toStdString());
        return Error(ErrorCode::InternalError, "Cannot open cache file: " + filePath.toStdString());
    }

    file.write(value);
    file.close();

    if (ttl.count() > 0) {
        QString ttlFilePath = getTtlFilePath(key);
        QFile ttlFile(ttlFilePath);
        if (ttlFile.open(QIODevice::WriteOnly)) {
            qint64 expiry = QDateTime::currentDateTime().addSecs(static_cast<int>(ttl.count())).toSecsSinceEpoch();
            ttlFile.write(QString::number(expiry).toUtf8());
            ttlFile.close();
        }
    }

    m_entrySizes[key] = value.size();
    m_currentSize += value.size();
    m_lruList.push_front(key);

    return {};
}

Result<QByteArray> DiskCache::get(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    QString filePath = getFilePath(key);

    if (!QFile::exists(filePath)) {
        m_entrySizes.erase(key);
        m_lruList.remove(key);
        return Error(ErrorCode::NotFound, "Cache key not found");
    }

    if (isExpired(key)) {
        remove(key);
        return Error(ErrorCode::NotFound, "Cache key expired");
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Error(ErrorCode::InternalError, "Cannot open cache file: " + filePath.toStdString());
    }

    m_lruList.remove(key);
    m_lruList.push_front(key);

    return file.readAll();
}

bool DiskCache::contains(const QString& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);

    QString filePath = getFilePath(key);

    if (!QFile::exists(filePath)) {
        return false;
    }

    return !isExpired(key);
}

Result<void> DiskCache::remove(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    QString filePath = getFilePath(key);
    QString ttlFilePath = getTtlFilePath(key);

    auto it = m_entrySizes.find(key);
    if (it != m_entrySizes.end()) {
        m_currentSize -= it->second;
        m_entrySizes.erase(it);
    }
    m_lruList.remove(key);

    bool removed = false;
    if (QFile::exists(filePath)) {
        removed = QFile::remove(filePath);
    }
    if (QFile::exists(ttlFilePath)) {
        QFile::remove(ttlFilePath);
    }

    if (!removed) {
        return Error(ErrorCode::NotFound, "Cache key not found");
    }
    return {};
}

void DiskCache::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);

    QDir dir(m_cacheDir);
    QStringList filters;
    filters << "*";
    QStringList files = dir.entryList(filters, QDir::Files);

    for (const QString& file : files) {
        dir.remove(file);
    }

    m_entrySizes.clear();
    m_lruList.clear();
    m_currentSize = 0;
}

size_t DiskCache::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_currentSize;
}

size_t DiskCache::maxSize() const {
    return m_maxSize;
}

QString DiskCache::getFilePath(const QString& key) const {
    QByteArray hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Sha256);
    QString hashString = hash.toHex();

    QString subDir = hashString.left(2);
    QString fileName = hashString.mid(2);

    return QString("%1/%2/%3").arg(m_cacheDir).arg(subDir).arg(fileName);
}

QString DiskCache::getTtlFilePath(const QString& key) const {
    return getFilePath(key) + ".ttl";
}

bool DiskCache::isExpired(const QString& key) const {
    QString ttlFilePath = getTtlFilePath(key);

    if (!QFile::exists(ttlFilePath)) {
        return false;
    }

    QFile ttlFile(ttlFilePath);
    if (!ttlFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QString expiryStr = QString::fromUtf8(ttlFile.readAll());
    bool ok = false;
    qint64 expiry = expiryStr.toLongLong(&ok);

    if (!ok) {
        return false;
    }

    return QDateTime::currentDateTime().toSecsSinceEpoch() > expiry;
}

void DiskCache::evict() {
    while (!m_lruList.empty()) {
        QString key = m_lruList.back();
        m_lruList.pop_back();

        QString filePath = getFilePath(key);
        QString ttlFilePath = getTtlFilePath(key);

        bool expired = isExpired(key);

        if (QFile::exists(filePath)) {
            QFile::remove(filePath);
        }
        if (QFile::exists(ttlFilePath)) {
            QFile::remove(ttlFilePath);
        }

        auto it = m_entrySizes.find(key);
        if (it != m_entrySizes.end()) {
            m_currentSize -= it->second;
            m_entrySizes.erase(it);
        }

        if (expired) {
            return;
        }
    }
}

}