#ifndef SOUL_STORAGE_CACHE_H
#define SOUL_STORAGE_CACHE_H

#include <QString>
#include <QByteArray>
#include <unordered_map>
#include <list>
#include <mutex>
#include <chrono>
#include "soul/core/interface.h"
#include "soul/core/result.h"

namespace sc {

template<typename K, typename V>
class ICache : public IInterface {
public:
    ~ICache() override = default;

    virtual Result<void> put(const K& key, const V& value) = 0;
    virtual Result<void> put(const K& key, const V& value, std::chrono::seconds ttl) = 0;
    virtual Result<V> get(const K& key) const = 0;
    virtual bool contains(const K& key) const = 0;
    virtual Result<void> remove(const K& key) = 0;
    virtual void clear() = 0;
    virtual size_t size() const = 0;
    virtual size_t maxSize() const = 0;

    std::string interfaceName() const override { return "ICache"; }
};

template<typename K, typename V>
class MemoryCache : public ICache<K, V> {
public:
    struct CacheEntry {
        V value;
        std::chrono::steady_clock::time_point expiry;
        bool hasTtl = false;
    };

    MemoryCache(size_t maxSize = 1024) : m_maxSize(maxSize) {}

    Result<void> put(const K& key, const V& value) override {
        return put(key, value, std::chrono::seconds(0));
    }

    Result<void> put(const K& key, const V& value, std::chrono::seconds ttl) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it != m_cache.end()) {
            m_list.erase(it->second.listIt);
        }

        if (m_cache.size() >= m_maxSize) {
            evict();
        }

        auto listIt = m_list.insert(m_list.begin(), key);
        CacheEntry entry;
        entry.value = value;
        if (ttl.count() > 0) {
            entry.expiry = std::chrono::steady_clock::now() + ttl;
            entry.hasTtl = true;
        }
        m_cache[key] = {entry, listIt};

        return {};
    }

    Result<V> get(const K& key) const override {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return Error(ErrorCode::NotFound, "Cache key not found");
        }

        const CacheEntry& entry = it->second.entry;
        if (entry.hasTtl && std::chrono::steady_clock::now() > entry.expiry) {
            return Error(ErrorCode::NotFound, "Cache key expired");
        }

        m_list.splice(m_list.begin(), m_list, it->second.listIt);
        return entry.value;
    }

    bool contains(const K& key) const override {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return false;
        }

        const CacheEntry& entry = it->second.entry;
        if (entry.hasTtl && std::chrono::steady_clock::now() > entry.expiry) {
            return false;
        }

        return true;
    }

    Result<void> remove(const K& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return Error(ErrorCode::NotFound, "Cache key not found");
        }

        m_list.erase(it->second.listIt);
        m_cache.erase(it);
        return {};
    }

    void clear() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_cache.clear();
        m_list.clear();
    }

    size_t size() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_cache.size();
    }

    size_t maxSize() const override {
        return m_maxSize;
    }

private:
    void evict() {
        while (!m_list.empty()) {
            const K& key = m_list.back();
            auto it = m_cache.find(key);

            if (it != m_cache.end()) {
                const CacheEntry& entry = it->second.entry;
                if (!entry.hasTtl || std::chrono::steady_clock::now() > entry.expiry) {
                    m_cache.erase(it);
                    m_list.pop_back();
                    return;
                }

                m_list.splice(m_list.begin(), m_list, it->second.listIt);
            }

            m_list.pop_back();
        }
    }

    mutable std::mutex m_mutex;
    size_t m_maxSize;

    struct CacheNode {
        CacheEntry entry;
        typename std::list<K>::iterator listIt;
    };

    std::unordered_map<K, CacheNode> m_cache;
    std::list<K> m_list;
};

class DiskCache : public ICache<QString, QByteArray> {
public:
    DiskCache(const QString& cacheDir, size_t maxSize = 1024 * 1024 * 100);
    ~DiskCache() override = default;

    Result<void> put(const QString& key, const QByteArray& value) override;
    Result<void> put(const QString& key, const QByteArray& value, std::chrono::seconds ttl) override;
    Result<QByteArray> get(const QString& key) const override;
    bool contains(const QString& key) const override;
    Result<void> remove(const QString& key) override;
    void clear() override;
    size_t size() const override;
    size_t maxSize() const override;

private:
    QString getFilePath(const QString& key) const;
    bool isExpired(const QString& key) const;

    QString m_cacheDir;
    size_t m_maxSize;
    mutable std::mutex m_mutex;
};

}

#endif