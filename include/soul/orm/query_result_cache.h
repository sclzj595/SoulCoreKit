#ifndef SOUL_ORM_QUERY_RESULT_CACHE_H
#define SOUL_ORM_QUERY_RESULT_CACHE_H

// ============================================================================
// query_result_cache.h — ORM 查询结果缓存 [v1.9.2 新增]
// ============================================================================
//
// 设计目标: 为 ORM 查询提供结果缓存,减少重复查询对数据库的压力。
// 支持基于表名+查询键的缓存失效策略。
//
// 设计原则:
//   - 缓存键: {table_name}:{query_key}
//   - 缓存失效: 写操作(INSERT/UPDATE/DELETE)自动失效对应表的所有缓存
//   - TTL: 支持可配置的缓存过期时间
//   - 线程安全: 内部使用 shared_mutex 保护
//
// 用法:
//   QueryResultCache cache(std::chrono::seconds(60));
//   cache.put("users:findById:42", userResult);
//   auto cached = cache.get("users:findById:42");
//   cache.invalidateTable("users");  // 写操作后失效

#include <chrono>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <optional>
#include "soul/core/result.h"

namespace sc {
namespace orm {

// ============================================================================
// CacheEntry — 缓存条目
// ============================================================================
template<typename T>
struct CacheEntry {
    T value;
    std::chrono::steady_clock::time_point createdAt;
    std::chrono::milliseconds ttl;

    bool isExpired() const {
        if (ttl.count() <= 0) return false;
        return std::chrono::steady_clock::now() > createdAt + ttl;
    }
};

// ============================================================================
// QueryResultCache — ORM 查询结果缓存
// ============================================================================
//
// @tparam T 缓存的值类型
// @thread_safety Thread-Safe — 内部使用 std::shared_mutex
template<typename T>
class QueryResultCache {
public:
    /// @brief 构造函数
    /// @param defaultTtl 默认过期时间(0 表示永不过期)
    explicit QueryResultCache(std::chrono::milliseconds defaultTtl = std::chrono::milliseconds(0))
        : m_defaultTtl(defaultTtl) {}

    /// @brief 存入缓存
    /// @param key      缓存键(格式: "{table}:{operation}:{params}")
    /// @param value    缓存值
    /// @param ttl      过期时间(默认使用全局 TTL)
    void put(const std::string& key, const T& value,
             std::optional<std::chrono::milliseconds> ttl = std::nullopt) {
        std::lock_guard<std::shared_mutex> lock(m_mutex);

        auto effectiveTtl = ttl.value_or(m_defaultTtl);
        CacheEntry<T> entry;
        entry.value = value;
        entry.createdAt = std::chrono::steady_clock::now();
        entry.ttl = effectiveTtl;

        m_cache[key] = std::move(entry);

        // 记录表名关联(用于批量失效)
        auto tableName = extractTableName(key);
        if (!tableName.empty()) {
            m_tableKeys[tableName].push_back(key);
        }
    }

    /// @brief 从缓存获取
    /// @param key 缓存键
    /// @return 缓存值(不存在或已过期时返回 nullopt)
    std::optional<T> get(const std::string& key) {
        std::shared_lock<std::shared_mutex> lock(m_mutex);

        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return std::nullopt;
        }

        if (it->second.isExpired()) {
            return std::nullopt;
        }

        return it->second.value;
    }

    /// @brief 失效指定键的缓存
    void invalidate(const std::string& key) {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        m_cache.erase(key);
    }

    /// @brief 失效指定表的所有缓存 [v1.9.2 新增]
    /// @param tableName 表名
    void invalidateTable(const std::string& tableName) {
        std::lock_guard<std::shared_mutex> lock(m_mutex);

        auto it = m_tableKeys.find(tableName);
        if (it != m_tableKeys.end()) {
            for (const auto& key : it->second) {
                m_cache.erase(key);
            }
            m_tableKeys.erase(it);
        }
    }

    /// @brief 清空所有缓存
    void clear() {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        m_cache.clear();
        m_tableKeys.clear();
    }

    /// @brief 获取缓存大小
    size_t size() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        return m_cache.size();
    }

    /// @brief 清理过期条目
    void evictExpired() {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        auto it = m_cache.begin();
        while (it != m_cache.end()) {
            if (it->second.isExpired()) {
                it = m_cache.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    /// @brief 从缓存键提取表名
    static std::string extractTableName(const std::string& key) {
        auto pos = key.find(':');
        if (pos != std::string::npos) {
            return key.substr(0, pos);
        }
        return {};
    }

    std::chrono::milliseconds m_defaultTtl;
    std::unordered_map<std::string, CacheEntry<T>> m_cache;
    std::unordered_map<std::string, std::vector<std::string>> m_tableKeys;  ///< 表名→缓存键列表
    mutable std::shared_mutex m_mutex;
};

} // namespace orm
} // namespace sc

#endif // SOUL_ORM_QUERY_RESULT_CACHE_H