#ifndef SOUL_CACHE_ICACHE_H
#define SOUL_CACHE_ICACHE_H

#include <chrono>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <vector>
#include "soul/core/result.h"

namespace sc {
namespace cache {

/**
 * @brief 缓存统计信息
 *
 * 记录缓存命中/未命中/淘汰等运行时指标,用于可观测性上报。
 *
 * @see ICache::stats()
 */
struct CacheStats {
    std::size_t hitCount = 0;
    std::size_t missCount = 0;
    std::size_t evictionCount = 0;
    std::size_t totalBytes = 0;

    /**
     * @brief 命中率,范围 [0.0, 1.0]
     * @return hitCount / (hitCount + missCount),无数据时返回 0.0
     */
    double hitRate() const noexcept {
        std::size_t total = hitCount + missCount;
        return total == 0 ? 0.0 : static_cast<double>(hitCount) / static_cast<double>(total);
    }
};

/**
 * @brief 缓存抽象接口
 *
 * 定义统一的缓存访问契约,L1(内存)/L2(磁盘)/MultiLevel 均实现此接口。
 * 上层只依赖 ICache,不感知具体实现,符合依赖倒置原则。
 *
 * @tparam K 键类型,需可哈希(std::hash<K>)且可比较(std::equal_to<K>)
 * @tparam V 值类型,需可拷贝/移动
 *
 * @thread_safety Thread-Safe — 所有实现必须线程安全(ADR-005 Level 2)
 */
template<typename K, typename V>
class ICache {
public:
    virtual ~ICache() = default;

    /**
     * @brief 同步获取缓存值
     * @param key 缓存键
     * @return Result 包裹的 optional<V>:
     *         - Ok + nullopt: 键不存在(未命中)
     *         - Ok + value:   命中
     *         - Error:        底层故障(如磁盘 IO 错误)
     */
    [[nodiscard]] virtual Result<std::optional<V>> get(const K& key) = 0;

    /**
     * @brief 同步写入缓存
     * @param key 缓存键
     * @param value 缓存值
     * @param ttl 可选存活时间;为 nullopt 时使用实现默认策略
     */
    virtual Result<void> put(const K& key, const V& value,
                             std::optional<std::chrono::milliseconds> ttl = std::nullopt) = 0;

    /**
     * @brief 移除指定键
     * @param key 缓存键
     */
    virtual Result<void> remove(const K& key) = 0;

    /**
     * @brief 检查键是否存在(未过期)
     * @param key 缓存键
     * @return Result<bool:存在性
     */
    [[nodiscard]] virtual Result<bool> contains(const K& key) = 0;

    /**
     * @brief 清空所有缓存条目
     */
    virtual Result<void> clear() = 0;

    /**
     * @brief 批量获取
     * @param keys 键集合
     * @return Result 包裹的 map:仅包含命中且未过期的键值对
     */
    [[nodiscard]] virtual Result<std::unordered_map<K, V>> getMany(const std::vector<K>& keys) = 0;

    /**
     * @brief 批量写入
     * @param entries 键值对集合
     * @param ttl 可选存活时间,应用于所有条目
     */
    virtual Result<void> putMany(const std::unordered_map<K, V>& entries,
                                 std::optional<std::chrono::milliseconds> ttl = std::nullopt) = 0;

    /**
     * @brief 当前缓存条目数
     * @note 返回的是某一时刻的快照,并发场景下可能已变化
     */
    [[nodiscard]] virtual std::size_t size() const = 0;

    /**
     * @brief 获取运行时统计
     * @return CacheStats 快照
     */
    [[nodiscard]] virtual CacheStats stats() const = 0;
};

} // namespace cache
} // namespace sc

#endif // SOUL_CACHE_ICACHE_H
