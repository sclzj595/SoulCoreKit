#ifndef SOUL_CACHE_REDIS_CACHE_H
#define SOUL_CACHE_REDIS_CACHE_H

// ============================================================================
// redis_cache.h — Redis Cache Adapter [v2.9.2 新增]
// ============================================================================
//
// 实现 ICache<string,string> 接口，后端为 Redis。
// 可作为 MultiLevelCache 的 L3 远程缓存层。
//
// 失败模型:
//   Redis 不可用 → get() 返回 Err(BackendUnavailable)
//   调用方可根据 Result 决定降级策略 (如 fallback 到 DiskCache)
//
// 设计原则:
//   - 接口不可见 Redis 具体实现 (调用方只依赖 ICache)
//   - 连接池管理 (单连接/连接池，按需)
//   - 序列化: 默认 string → string，可注入 ISerializer
//
// 依赖:
//   - 需要 Redis 客户端库 (如 hiredis)
//   - v2.9.2 提供完整接口 + Stub 实现
//   - 编译选项: SOULCOREKIT_ENABLE_REDIS=ON
//
// 用法:
//   #ifdef SOUL_ENABLE_REDIS
//   auto redis = std::make_shared<RedisCache>("tcp://127.0.0.1:6379");
//   redis->setKeyPrefix("myapp:");
//   redis->setDefaultTtl(std::chrono::minutes(10));
//
//   auto& health = HealthAggregator::instance();
//   health.registerIndicator(redis->createHealthIndicator());
//
//   auto result = redis->get("user:42");
//   if (result.isErr()) {
//       // Redis 不可用 → 降级到 DiskCache
//   }
//   #endif

#include "soul/cache/icache.h"
#include <QString>
#include <memory>
#include <string>
#include <mutex>
#include <atomic>

namespace sc {
namespace cache {

// ============================================================================
// RedisCache — Redis 缓存适配器
// ============================================================================

class RedisCache : public ICache<std::string, std::string> {
public:
    /// @brief 构造
    /// @param connectionString Redis 连接字符串 (tcp://host:port 或 unix://path)
    explicit RedisCache(const std::string& connectionString);

    ~RedisCache() override;

    // === ICache 接口 ===

    Result<std::optional<std::string>> get(const std::string& key) override;
    Result<void> put(const std::string& key, const std::string& value,
                     std::optional<std::chrono::milliseconds> ttl = std::nullopt) override;
    Result<void> remove(const std::string& key) override;
    Result<bool> contains(const std::string& key) override;
    Result<void> clear() override;
    Result<std::unordered_map<std::string, std::string>>
        getMany(const std::vector<std::string>& keys) override;
    Result<void> putMany(const std::unordered_map<std::string, std::string>& entries,
                         std::optional<std::chrono::milliseconds> ttl = std::nullopt) override;
    std::size_t size() const override;
    CacheStats stats() const override;

    // === RedisCache 特有 ===

    /// @brief 设置键前缀 (如 "myapp:" → key = "myapp:user:42")
    void setKeyPrefix(const std::string& prefix);

    /// @brief 设置默认 TTL (nullopt = 永不过期)
    void setDefaultTtl(std::optional<std::chrono::milliseconds> ttl);

    /// @brief 创建 HealthIndicator (用于 HealthAggregator)
    /// @return IHealthIndicator 的 shared_ptr
    /// @note 需要 include soul/core/health.h
    std::shared_ptr<class IHealthIndicator> createHealthIndicator();

    /// @brief 测试连接
    Result<void> ping();

private:
    std::string makeKey(const std::string& key) const;

    std::string m_connectionString;
    std::string m_keyPrefix;
    std::optional<std::chrono::milliseconds> m_defaultTtl;
    mutable CacheStats m_stats;
    mutable std::mutex m_mutex;
    std::atomic<bool> m_connected{false};

    // 实际 Redis 客户端 (v2.9.2: Stub — 需要 hiredis 或其他库)
    // 当 SOUL_ENABLE_REDIS 启用时，此指针指向真实客户端
    void* m_client = nullptr;  // 类型擦除，避免头文件依赖 hiredis
};

} // namespace cache
} // namespace sc

#endif // SOUL_CACHE_REDIS_CACHE_H
