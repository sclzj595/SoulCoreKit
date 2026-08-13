#ifndef SOUL_CORE_HEALTH_H
#define SOUL_CORE_HEALTH_H

// ============================================================================
// health.h — 统一健康检查模型 [v2.8.0 新增]
// ============================================================================
//
// 设计原则:
//   - Health 模块与 HTTP Endpoint 解耦 (可在 CLI/CS/BS 中独立使用)
//   - 组件通过 IHealthIndicator 接口注册健康检查
//   - HealthAggregator 聚合所有指标，生成统一状态
//
// 三层健康检查:
//   Liveness  (/live)  — 进程是否存活
//   Readiness (/ready) — 是否可以处理请求
//   Health    (/health)— 完整诊断信息 (含依赖状态)
//
// 用法:
//   auto& aggregator = HealthAggregator::instance();
//   aggregator.registerIndicator("database", dbHealthIndicator);
//   aggregator.registerIndicator("cache", cacheHealthIndicator);
//
//   auto status = aggregator.check();  // 聚合所有指标
//
// 输出:
//   {
//     "status": "UP",
//     "dependencies": {
//       "database": {"status": "UP", "latency_ms": 2},
//       "cache":    {"status": "UP"}
//     }
//   }

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sc {

// ============================================================================
// HealthStatus — 单个健康检查结果
// ============================================================================

enum class HealthStatusCode {
    Up,          // 健康
    Down,        // 不可用
    Degraded,    // 降级 (部分功能受限)
    Unknown      // 未知
};

struct HealthStatus {
    QString name;            // 组件名称
    HealthStatusCode code = HealthStatusCode::Unknown;
    QString message;         // 人类可读消息
    QJsonObject details;     // 额外详情 (如 latency_ms, pool_size)

    bool isUp() const { return code == HealthStatusCode::Up; }
    bool isDown() const { return code == HealthStatusCode::Down; }

    static HealthStatus up(const QString& name, const QString& msg = "OK") {
        return {name, HealthStatusCode::Up, msg, {}};
    }

    static HealthStatus down(const QString& name, const QString& msg) {
        return {name, HealthStatusCode::Down, msg, {}};
    }

    static HealthStatus degraded(const QString& name, const QString& msg) {
        return {name, HealthStatusCode::Degraded, msg, {}};
    }
};

// ============================================================================
// IHealthIndicator — 健康指标接口
// ============================================================================

class IHealthIndicator {
public:
    virtual ~IHealthIndicator() = default;

    /// @brief 执行健康检查
    /// @return 健康状态
    virtual HealthStatus check() = 0;

    /// @return 组件名称 (如 "database", "cache")
    virtual std::string name() const = 0;
};

// ============================================================================
// HealthAggregator — 健康检查聚合器 (单例)
// ============================================================================
//
// 管理所有 IHealthIndicator，聚合为统一状态。
// 线程安全: 注册加锁，检查不加锁 (快照模式)。

class HealthAggregator {
public:
    static HealthAggregator& instance();

    /// @brief 注册健康指标
    void registerIndicator(std::shared_ptr<IHealthIndicator> indicator);

    /// @brief 注销健康指标
    void unregisterIndicator(const std::string& name);

    /// @brief 执行所有健康检查
    /// @return 聚合后的 HealthStatus 列表
    std::vector<HealthStatus> checkAll();

    /// @brief 获取整体状态 (最差的组件状态)
    /// UP > DEGRADED > DOWN > UNKNOWN
    HealthStatusCode overallStatus();

    /// @brief 检查是否健康 (所有组件 UP)
    bool isHealthy();

    /// @brief 检查是否就绪 (至少没有 DOWN 的组件)
    bool isReady();

    /// @brief 生成 JSON 响应 (用于 /health endpoint)
    QByteArray toJson();

private:
    HealthAggregator() = default;
    std::vector<std::shared_ptr<IHealthIndicator>> m_indicators;
    std::mutex m_mutex;
};

// ============================================================================
// LambdaHealthIndicator — Lambda 快捷健康指标
// ============================================================================
//
// 用法:
//   auto dbHealth = LambdaHealthIndicator::create("database", []() {
//       return HealthStatus::up("database");
//   });

class LambdaHealthIndicator : public IHealthIndicator {
public:
    using CheckFn = std::function<HealthStatus()>;

    static std::shared_ptr<LambdaHealthIndicator>
    create(const std::string& name, CheckFn fn) {
        return std::make_shared<LambdaHealthIndicator>(name, std::move(fn));
    }

    LambdaHealthIndicator(const std::string& name, CheckFn fn)
        : m_name(name), m_fn(std::move(fn)) {}

    HealthStatus check() override { return m_fn(); }
    std::string name() const override { return m_name; }

private:
    std::string m_name;
    CheckFn m_fn;
};

// ============================================================================
// Built-in Health Indicators
// ============================================================================

/// @brief 基础存活检查 (始终 UP)
class LivenessIndicator : public IHealthIndicator {
public:
    HealthStatus check() override {
        return HealthStatus::up("liveness", "Process is alive");
    }
    std::string name() const override { return "liveness"; }
};

/// @brief 内存压力检查
class MemoryHealthIndicator : public IHealthIndicator {
public:
    /// @param maxUsageMB 最大内存使用阈值 (MB)，超过则 DEGRADED
    explicit MemoryHealthIndicator(uint64_t maxUsageMB = 0);

    HealthStatus check() override;
    std::string name() const override { return "memory"; }

private:
    uint64_t m_maxUsageMB;
    static uint64_t currentMemoryUsageMB();
};

} // namespace sc

#endif // SOUL_CORE_HEALTH_H
