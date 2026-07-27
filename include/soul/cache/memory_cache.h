#ifndef SOUL_CACHE_MEMORY_CACHE_H
#define SOUL_CACHE_MEMORY_CACHE_H

#include <chrono>
#include <cstddef>
#include <list>
#include <mutex>
#include <optional>
#include <unordered_map>
#include <vector>
#include "soul/cache/icache.h"
#include "soul/cache/size_estimator.h"
#include "soul/core/error.h"

namespace sc {
namespace cache {

/**
 * @brief L1 内存缓存
 *
 * 基于 std::unordered_map + std::list 实现 LRU(最近最少使用)淘汰策略,
 * 同时支持 TTL(存活时间)过期。线程安全,适合高频读写场景。
 *
 * @par 配置
 * 通过 Config 控制:
 * - maxEntries: 最大条目数(默认 10000)
 * - maxBytes:   最大字节数(默认 100MB)
 * - defaultTtl: 默认 TTL(为 nullopt 时永不过期)
 *
 * @par 淘汰顺序
 * 当达到 maxEntries 或 maxBytes 时,优先淘汰:
 * 1. 已过期的条目
 * 2. 最久未访问的条目(LRU)
 *
 * @thread_safety Thread-Safe — 内部使用 std::mutex 同步所有访问
 */
template<typename K, typename V>
class MemoryCache : public ICache<K, V> {
public:
    /**
     * @brief 配置参数
     */
    struct Config {
        std::size_t maxEntries = 10000;
        std::size_t maxBytes = 100 * 1024 * 1024;  // 100MB
        std::optional<std::chrono::milliseconds> defaultTtl;
    };

    /**
     * @brief 构造函数
     * @param config 配置参数
     */
    explicit MemoryCache(Config config = {})
        : m_config(std::move(config)) {}

    ~MemoryCache() override = default;

    MemoryCache(const MemoryCache&) = delete;
    MemoryCache& operator=(const MemoryCache&) = delete;
    MemoryCache(MemoryCache&&) = delete;
    MemoryCache& operator=(MemoryCache&&) = delete;

    [[nodiscard]] Result<std::optional<V>> get(const K& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_entries.find(key);
        if (it == m_entries.end()) {
            m_stats.missCount++;
            return std::optional<V>{};
        }

        // TTL 过期检查
        if (isExpiredUnlocked(it->second)) {
            evictEntryUnlocked(it);
            m_stats.missCount++;
            return std::optional<V>{};
        }

        // LRU: 移动到链表头部(最近使用)
        touchUnlocked(it);

        m_stats.hitCount++;
        return std::optional<V>{it->second.value};
    }

    Result<void> put(const K& key, const V& value,
                     std::optional<std::chrono::milliseconds> ttl = std::nullopt) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        const std::size_t entrySize = estimateSize(value);
        const auto expireTime = computeExpireTime(ttl);

        auto it = m_entries.find(key);
        if (it != m_entries.end()) {
            // 已存在:更新值并调整大小
            m_stats.totalBytes -= it->second.size;
            it->second.value = value;
            it->second.size = entrySize;
            it->second.expireTime = expireTime;
            it->second.accessCount++;
            touchUnlocked(it);
            m_stats.totalBytes += entrySize;
            return {};
        }

        // 新增:先淘汰以满足容量约束
        evictToFitUnlocked(entrySize);

        typename std::list<K>::iterator listIt = m_lruList.insert(m_lruList.begin(), key);
        Entry entry;
        entry.value = value;
        entry.size = entrySize;
        entry.expireTime = expireTime;
        entry.accessCount = 1;
        entry.listIt = listIt;

        m_stats.totalBytes += entrySize;
        m_entries.emplace(key, std::move(entry));
        return {};
    }

    Result<void> remove(const K& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_entries.find(key);
        if (it == m_entries.end()) {
            return Error(ErrorCode::NotFound, "Cache key not found");
        }
        evictEntryUnlocked(it);
        return {};
    }

    [[nodiscard]] Result<bool> contains(const K& key) override {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_entries.find(key);
        if (it == m_entries.end()) {
            return false;
        }
        if (isExpiredUnlocked(it->second)) {
            evictEntryUnlocked(it);
            return false;
        }
        return true;
    }

    Result<void> clear() override {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_entries.clear();
        m_lruList.clear();
        m_stats.totalBytes = 0;
        return {};
    }

    [[nodiscard]] Result<std::unordered_map<K, V>> getMany(const std::vector<K>& keys) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::unordered_map<K, V> result;
        result.reserve(keys.size());

        for (const K& key : keys) {
            auto it = m_entries.find(key);
            if (it == m_entries.end()) {
                m_stats.missCount++;
                continue;
            }
            if (isExpiredUnlocked(it->second)) {
                evictEntryUnlocked(it);
                m_stats.missCount++;
                continue;
            }
            touchUnlocked(it);
            m_stats.hitCount++;
            result.emplace(key, it->second.value);
        }
        return result;
    }

    Result<void> putMany(const std::unordered_map<K, V>& entries,
                         std::optional<std::chrono::milliseconds> ttl = std::nullopt) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto expireTime = computeExpireTime(ttl);

        for (const auto& [key, value] : entries) {
            const std::size_t entrySize = estimateSize(value);

            auto it = m_entries.find(key);
            if (it != m_entries.end()) {
                m_stats.totalBytes -= it->second.size;
                it->second.value = value;
                it->second.size = entrySize;
                it->second.expireTime = expireTime;
                it->second.accessCount++;
                touchUnlocked(it);
                m_stats.totalBytes += entrySize;
                continue;
            }

            evictToFitUnlocked(entrySize);

            typename std::list<K>::iterator listIt = m_lruList.insert(m_lruList.begin(), key);
            Entry entry;
            entry.value = value;
            entry.size = entrySize;
            entry.expireTime = expireTime;
            entry.accessCount = 1;
            entry.listIt = listIt;

            m_stats.totalBytes += entrySize;
            m_entries.emplace(key, std::move(entry));
        }
        return {};
    }

    [[nodiscard]] std::size_t size() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_entries.size();
    }

    [[nodiscard]] CacheStats stats() const override {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stats;
    }

private:
    struct Entry {
        V value;
        std::chrono::steady_clock::time_point expireTime;
        std::size_t size = 0;
        std::size_t accessCount = 0;
        typename std::list<K>::iterator listIt;
    };

    using EntryMap = std::unordered_map<K, Entry>;

    bool isExpiredUnlocked(const Entry& entry) const noexcept {
        // expireTime == max 时 now > max 永远为 false,无需特判
        return std::chrono::steady_clock::now() > entry.expireTime;
    }

    std::chrono::steady_clock::time_point
    computeExpireTime(std::optional<std::chrono::milliseconds> ttl) const {
        std::chrono::milliseconds effectiveTtl = ttl.value_or(
            m_config.defaultTtl.value_or(std::chrono::milliseconds::zero()));
        if (effectiveTtl.count() <= 0) {
            return std::chrono::steady_clock::time_point::max();
        }
        return std::chrono::steady_clock::now() + effectiveTtl;
    }

    void touchUnlocked(typename EntryMap::iterator it) {
        m_lruList.splice(m_lruList.begin(), m_lruList, it->second.listIt);
    }

    void evictEntryUnlocked(typename EntryMap::iterator it) {
        m_lruList.erase(it->second.listIt);
        m_stats.totalBytes -= it->second.size;
        m_stats.evictionCount++;
        m_entries.erase(it);
    }

    void evictToFitUnlocked(std::size_t incomingSize) {
        while (!m_entries.empty()) {
            const bool overEntries = m_entries.size() >= m_config.maxEntries;
            const bool overBytes =
                m_config.maxBytes > 0 &&
                (m_stats.totalBytes + incomingSize) > m_config.maxBytes;
            if (!overEntries && !overBytes) {
                break;
            }
            evictLruUnlocked();
        }
    }

    void evictLruUnlocked() {
        if (m_lruList.empty()) {
            return;
        }
        const K& lruKey = m_lruList.back();
        auto it = m_entries.find(lruKey);
        if (it != m_entries.end()) {
            evictEntryUnlocked(it);
        } else {
            m_lruList.pop_back();
        }
    }

    std::size_t estimateSize(const V& value) const noexcept {
        return SizeEstimator<V>::estimate(value);
    }

    Config m_config;
    EntryMap m_entries;
    std::list<K> m_lruList;
    mutable std::mutex m_mutex;
    mutable CacheStats m_stats;
};

} // namespace cache
} // namespace sc

#endif // SOUL_CACHE_MEMORY_CACHE_H
