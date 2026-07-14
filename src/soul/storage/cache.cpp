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

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        SC_ERROR("Cannot open cache file: " + filePath.toStdString());
        return Error(ErrorCode::InternalError, "Cannot open cache file: " + filePath.toStdString());
    }

    file.write(value);
    file.close();

    if (ttl.count() > 0) {
        QString ttlFilePath = filePath + ".ttl";
        QFile ttlFile(ttlFilePath);
        if (ttlFile.open(QIODevice::WriteOnly)) {
            qint64 expiry = QDateTime::currentDateTime().addSecs(static_cast<int>(ttl.count())).toSecsSinceEpoch();
            ttlFile.write(QString::number(expiry).toUtf8());
            ttlFile.close();
        }
    }

    return {};
}

Result<QByteArray> DiskCache::get(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);

    QString filePath = getFilePath(key);

    if (!QFile::exists(filePath)) {
        return Error(ErrorCode::NotFound, "Cache key not found");
    }

    if (isExpired(key)) {
        return Error(ErrorCode::NotFound, "Cache key expired");
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return Error(ErrorCode::InternalError, "Cannot open cache file: " + filePath.toStdString());
    }

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
    QString ttlFilePath = filePath + ".ttl";

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
}

size_t DiskCache::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    QDir dir(m_cacheDir);
    QStringList filters;
    filters << "*";
    QStringList files = dir.entryList(filters, QDir::Files);

    return files.size();
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

bool DiskCache::isExpired(const QString& key) const {
    QString filePath = getFilePath(key);
    QString ttlFilePath = filePath + ".ttl";

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

}
