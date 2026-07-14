#include "soul/storage/file_storage.h"
#include "soul/core/error.h"
#include <QFile>
#include <QDir>
#include <QCryptographicHash>

namespace sc {

FileStorage::FileStorage() : m_isOpen(false) {
}

Result<void> FileStorage::open(const QString& path) {
    m_basePath = path;
    QDir dir(path);
    if (!dir.exists()) {
        if (!dir.mkpath(path)) {
            return Error(ErrorCode::InternalError, "Failed to create directory: " + path.toStdString());
        }
    }
    m_isOpen = true;
    return {};
}

void FileStorage::close() {
    m_isOpen = false;
}

bool FileStorage::isOpen() const {
    return m_isOpen;
}

QString FileStorage::getFilePath(const QString& key) const {
    QString hash = QCryptographicHash::hash(key.toUtf8(), QCryptographicHash::Md5).toHex();
    return m_basePath + "/" + hash;
}

Result<void> FileStorage::put(const QString& key, const QString& value) {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    QFile file(getFilePath(key));
    if (!file.open(QIODevice::WriteOnly)) {
        return Error(ErrorCode::InternalError, "Failed to open file for writing");
    }
    file.write(value.toUtf8());
    return {};
}

Result<QString> FileStorage::get(const QString& key) const {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    QFile file(getFilePath(key));
    if (!file.open(QIODevice::ReadOnly)) {
        return Error(ErrorCode::NotFound, "Key not found");
    }
    return QString::fromUtf8(file.readAll());
}

Result<void> FileStorage::remove(const QString& key) {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    if (!QFile::remove(getFilePath(key))) {
        return Error(ErrorCode::InternalError, "Failed to remove file");
    }
    return {};
}

bool FileStorage::contains(const QString& key) const {
    return QFile::exists(getFilePath(key));
}

Result<void> FileStorage::putBytes(const QString& key, const QByteArray& value) {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    QFile file(getFilePath(key));
    if (!file.open(QIODevice::WriteOnly)) {
        return Error(ErrorCode::InternalError, "Failed to open file for writing");
    }
    file.write(value);
    return {};
}

Result<QByteArray> FileStorage::getBytes(const QString& key) const {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    QFile file(getFilePath(key));
    if (!file.open(QIODevice::ReadOnly)) {
        return Error(ErrorCode::NotFound, "Key not found");
    }
    return file.readAll();
}

std::vector<QString> FileStorage::keys() const {
    std::vector<QString> result;
    QDir dir(m_basePath);
    QStringList entries = dir.entryList(QDir::Files);
    for (const QString& entry : entries) {
        result.push_back(entry);
    }
    return result;
}

int FileStorage::count() const {
    QDir dir(m_basePath);
    return dir.entryList(QDir::Files).size();
}

Result<void> FileStorage::clear() {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    QDir dir(m_basePath);
    QStringList entries = dir.entryList(QDir::Files);
    for (const QString& entry : entries) {
        dir.remove(entry);
    }
    return {};
}

}