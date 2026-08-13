#ifndef SOUL_CACHE_ICACHE_H
#define SOUL_CACHE_ICACHE_H

// ============================================================================
// icache.h — 缓存抽象接口 [v2.5.0 / v2.9.2 增强]
// ============================================================================
//
// v2.9.2 变更:
//   - CacheStats 新增: putCount, removeCount, errorCount, lastError, name
//   - 文档增强: 失败模型 (BackendUnavailable vs CacheMiss)
//   - 向后兼容: 所有现有实现无需修改
//
// 失败模型 (v2.9.2 明确):
//   get() → Result<optional<V>>
//     - Ok + has_value:  命中
//     - Ok + nullopt:    未命中 (正常，非错误)
//     - Err:             后端不可用 (Redis 断连、磁盘故障)
//   put() → Result<void>
//     - Ok:              写入成功
//     - Err:             后端不可用 (写入失败，数据丢失)
//
// 设计原则:
//   - 接口不暴露后端具体错误 (调用方不依赖 Redis/Kafka 等)
//   - 后端不可用 → Err(BackendUnavailable)，非 nullopt
//   - 调用方可以根据 Result 决定降级策略

#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "soul/core/result.h"

namespace sc {
namespace cache {

/**
 * @brief 缓存统计信息 [v2.9.2 增强]
 */
struct CacheStats {
    std::size_t hitCount = 0;
    std::size_t missCount = 0;
    std::size_t evictionCount = 0;
    std::size_t totalBytes = 0;

    // v2.9.2 新增
    std::size_t putCount = 0;     // 写入次数
    std::size_t removeCount = 0;  // 删除次数
    std::size_t errorCount = 0;   // 后端故障次数
    std::string lastError;        // 最近一次错误消息 (用于诊断)
    std::string name;             // 缓存实例名称

    double hitRate() const noexcept {
        std::size_t total = hitCount + missCount;
        return total == 0 ? 0.0 : static_cast<double>(hitCount) / static_cast<double>(total);
    }
};

/**
 * @brief 缓存抽象接口
 *
 * @tparam K 键类型 (需可哈希 + 可比较)
 * @tparam V 值类型 (需可拷贝/移动)
 *
 * @thread_safety Thread-Safe — 所有实现必须线程安全
 */
template<typename K, typename V>
class ICache {
public:
    virtual ~ICache() = default;

    /// @brief 获取缓存值
    /// @return Result<optional<V>>:
    ///   Ok + has_value: 命中
    ///   Ok + nullopt:   未命中 (正常)
    ///   Err:            后端不可用
    [[nodiscard]] virtual Result<std::optional<V>> get(const K& key) = 0;

    /// @brief 写入缓存
    /// @param ttl 存活时间 (nullopt = 实现默认)
    virtual Result<void> put(const K& key, const V& value,
                             std::optional<std::chrono::milliseconds> ttl = std::nullopt) = 0;

    /// @brief 移除指定键
    virtual Result<void> remove(const K& key) = 0;

    /// @brief 检查键是否存在 (未过期)
    [[nodiscard]] virtual Result<bool> contains(const K& key) = 0;

    /// @brief 清空所有条目
    virtual Result<void> clear() = 0;

    /// @brief 批量获取
    [[nodiscard]] virtual Result<std::unordered_map<K, V>> getMany(const std::vector<K>& keys) = 0;

    /// @brief 批量写入
    virtual Result<void> putMany(const std::unordered_map<K, V>& entries,
                                 std::optional<std::chrono::milliseconds> ttl = std::nullopt) = 0;

    /// @brief 当前条目数 (快照)
    [[nodiscard]] virtual std::size_t size() const = 0;

    /// @brief 运行时统计
    [[nodiscard]] virtual CacheStats stats() const = 0;
};

} // namespace cache
} // namespace sc

#endif // SOUL_CACHE_ICACHE_H
