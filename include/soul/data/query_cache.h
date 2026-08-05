#ifndef SOUL_DATA_QUERY_CACHE_H
#define SOUL_DATA_QUERY_CACHE_H

// ============================================================================
// query_cache.h — 查询结果缓存 [v2.0.0 新增]
// ============================================================================
//
// 提供带 TTL 和 LRU 淘汰策略的线程安全查询结果缓存。
//
// 核心设计:
//   - QueryCache<Key, Value> 模板类
//   - put(key, value, ttl): 存入缓存（带过期时间）
//   - get(key): 获取缓存（自动检查 TTL）
//   - invalidate(key): 手动失效
//   - clear(): 清空所有缓存
//   - size(): 缓存条目数
//   - 使用 LRU 淘汰策略，超出容量时淘汰最久未使用条目
//   - 线程安全：内部使用 std::mutex 保护
//
// 用法:
//   QueryCache<QString, std::vector<User>> cache(100);  // 最大 100 条
//   cache.put("key1", users, std::chrono::seconds(60));  // 60 秒 TTL
//   auto result = cache.get("key1");
//   if (result.has_value()) {
//       auto& users = result.value();
//   }

#include <algorithm>
#include <chrono>
#include <list>
#include <map>
#include <mutex>
#include <optional>
#include <utility>

namespace sc {
namespace data {

// ============================================================================
// QueryCache — 带 TTL + LRU 的线程安全缓存
// ============================================================================
///
/// @brief 查询结果缓存模板类
///
/// 支持 TTL（生存时间）和 LRU（最近最少使用）淘汰策略。
/// 所有公共方法均为线程安全。
///
/// @tparam Key   缓存键类型（需支持 std::map 的 Key 要求）
/// @tparam Value 缓存值类型（需支持拷贝构造）
///
/// @par 使用示例
/// @code
/// QueryCache<QString, std::vector<User>> cache(100);
/// cache.put("users_active", userList, std::chrono::seconds(30));
/// auto opt = cache.get("users_active");
/// if (opt.has_value()) {
///     process(opt.value());
/// }
/// @endcode
///
/// @thread_safety Thread-Safe — 所有公共方法加锁保护
template<typename Key, typename Value>
class QueryCache {
public:
    /// @brief 构造函数
    /// @param maxSize 最大缓存条目数（超出后按 LRU 淘汰）
    explicit QueryCache(size_t maxSize = 256)
        : m_maxSize(maxSize) {}

    ~QueryCache() = default;

    QueryCache(const QueryCache&) = delete;
    QueryCache& operator=(const QueryCache&) = delete;
    QueryCache(QueryCache&&) = delete;
    QueryCache& operator=(QueryCache&&) = delete;

    // ========================================================================
    // put — 存入缓存
    // ========================================================================

    /// @brief 存入缓存条目
    /// @param key 缓存键
    /// @param value 缓存值
    /// @param ttl 生存时间（过期后 get 返回 nullopt）
    void put(const Key& key, const Value& value,
             std::chrono::milliseconds ttl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        putInternal(key, value, ttl);
    }

    /// @brief 存入缓存条目（移动语义）
    /// @param key 缓存键
    /// @param value 缓存值
    /// @param ttl 生存时间
    void put(const Key& key, Value&& value,
             std::chrono::milliseconds ttl) {
        std::lock_guard<std::mutex> lock(m_mutex);
        putInternal(key, std::move(value), ttl);
    }

    // ========================================================================
    // get — 获取缓存
    // ========================================================================

    /// @brief 获取缓存条目
    /// @param key 缓存键
    /// @return 若存在且未过期返回 std::optional<Value>；否则返回 std::nullopt
    std::optional<Value> get(const Key& key) {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto it = m_index.find(key);
        if (it == m_index.end()) {
            return std::nullopt;
        }

        auto& entry = *it->second;
        // 检查 TTL 是否过期
        if (isExpired(entry)) {
            evict(it);
            return std::nullopt;
        }

        // 移动到 LRU 列表头部（最近使用）
        moveToFront(it->second);
        return entry.value;
    }

    // ========================================================================
    // invalidate — 失效指定条目
    // ========================================================================

    /// @brief 手动失效指定缓存条目
    /// @param key 缓存键
    void invalidate(const Key& key) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_index.find(key);
        if (it != m_index.end()) {
            evict(it);
        }
    }

    // ========================================================================
    // clear — 清空缓存
    // ========================================================================

    /// @brief 清空所有缓存条目
    void clear() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_lruList.clear();
        m_index.clear();
    }

    // ========================================================================
    // size — 缓存条目数
    // ========================================================================

    /// @brief 获取当前缓存条目数
    /// @return 缓存条目数
    [[nodiscard]] size_t size() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_index.size();
    }

    /// @brief 获取最大容量
    /// @return 最大容量
    [[nodiscard]] size_t maxSize() const noexcept { return m_maxSize; }

    /// @brief 设置最大容量
    /// @param size 新容量（若小于当前条目数，将淘汰最久未使用的条目）
    void setMaxSize(size_t size) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_maxSize = size;
        while (m_index.size() > m_maxSize) {
            evictOldest();
        }
    }

private:
    // ========================================================================
    // 内部数据结构
    // ========================================================================

    /// @brief 缓存条目
    struct CacheEntry {
        Key key;
        Value value;
        std::chrono::steady_clock::time_point expiresAt;

        CacheEntry(Key k, Value v, std::chrono::milliseconds ttl)
            : key(std::move(k))
            , value(std::move(v))
            , expiresAt(std::chrono::steady_clock::now() + ttl) {}
    };

    using LruIterator = typename std::list<CacheEntry>::iterator;

    // ========================================================================
    // 内部方法（调用前需持有锁）
    // ========================================================================

    template<typename V>
    void putInternal(const Key& key, V&& value, std::chrono::milliseconds ttl) {
        auto it = m_index.find(key);
        if (it != m_index.end()) {
            // 更新已存在的条目
            it->second->value = std::forward<V>(value);
            it->second->expiresAt = std::chrono::steady_clock::now() + ttl;
            moveToFront(it->second);
            return;
        }

        // 超容量则淘汰最久未使用条目
        if (m_index.size() >= m_maxSize) {
            evictOldest();
        }

        // 插入新条目
        m_lruList.emplace_front(key, std::forward<V>(value), ttl);
        m_index[key] = m_lruList.begin();
    }

    /// @brief 检查条目是否过期
    bool isExpired(const CacheEntry& entry) const {
        return std::chrono::steady_clock::now() >= entry.expiresAt;
    }

    /// @brief 将条目移动到 LRU 列表头部（最近使用）
    void moveToFront(LruIterator it) {
        m_lruList.splice(m_lruList.begin(), m_lruList, it);
    }

    /// @brief 淘汰指定条目
    void evict(typename std::map<Key, LruIterator>::iterator indexIt) {
        m_lruList.erase(indexIt->second);
        m_index.erase(indexIt);
    }

    /// @brief 淘汰最久未使用的条目（LRU 列表尾部）
    void evictOldest() {
        if (!m_lruList.empty()) {
            m_index.erase(m_lruList.back().key);
            m_lruList.pop_back();
        }
    }

    size_t m_maxSize;
    std::list<CacheEntry> m_lruList;               ///< LRU 链表（头部=最近使用）
    std::map<Key, LruIterator> m_index;            ///< 键到链表迭代器的映射
    mutable std::mutex m_mutex;                    ///< 线程安全互斥锁
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_QUERY_CACHE_H