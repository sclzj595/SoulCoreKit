#include "soul/storage/memory_storage.h"
#include "soul/core/error.h"

namespace sc {

MemoryStorage::MemoryStorage() : m_isOpen(false) {
}

Result<void> MemoryStorage::open(const QString& path) {
    Q_UNUSED(path);
    m_isOpen = true;
    return {};
}

void MemoryStorage::close() {
    m_isOpen = false;
}

bool MemoryStorage::isOpen() const {
    return m_isOpen;
}

Result<void> MemoryStorage::put(const QString& key, const QString& value) {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    m_stringData[key] = value;
    return {};
}

Result<QString> MemoryStorage::get(const QString& key) const {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    if (m_stringData.contains(key)) {
        return m_stringData[key];
    }
    return Error(ErrorCode::NotFound, "Key not found");
}

Result<void> MemoryStorage::remove(const QString& key) {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    m_stringData.remove(key);
    return {};
}

bool MemoryStorage::contains(const QString& key) const {
    return m_stringData.contains(key);
}

Result<void> MemoryStorage::putBytes(const QString& key, const QByteArray& value) {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    m_bytesData[key] = value;
    return {};
}

Result<QByteArray> MemoryStorage::getBytes(const QString& key) const {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    if (m_bytesData.contains(key)) {
        return m_bytesData[key];
    }
    return Error(ErrorCode::NotFound, "Key not found");
}

std::vector<QString> MemoryStorage::keys() const {
    std::vector<QString> result;
    for (auto it = m_stringData.begin(); it != m_stringData.end(); ++it) {
        result.push_back(it.key());
    }
    return result;
}

int MemoryStorage::count() const {
    return m_stringData.size();
}

Result<void> MemoryStorage::clear() {
    if (!m_isOpen) {
        return Error(ErrorCode::InternalError, "Storage not open");
    }
    m_stringData.clear();
    m_bytesData.clear();
    return {};
}

}