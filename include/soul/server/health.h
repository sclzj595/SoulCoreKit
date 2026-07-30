#ifndef SOUL_SERVER_HEALTH_H
#define SOUL_SERVER_HEALTH_H

// ============================================================================
// health.h — Server 端健康检查端点
// ============================================================================
//
// 设计目标: 对标 SpringBoot Actuator HealthEndpoint,提供 CS 架构 Server 端
// 进程探活接口,供 Client 端或运维系统检测服务可用性。
//
// 核心概念:
//   - IHealthIndicator: 健康检查指示器接口,各模块实现自己的健康检查
//   - HealthEndpoint:   聚合所有指示器,提供 liveness/readiness 两种探针
//   - HealthStatus:     健康状态枚举(UP/DOWN/UNKNOWN)
//
// 探针类型:
//   - Liveness:  进程存活检查(快速,仅检查关键依赖,如进程是否响应)
//   - Readiness: 服务就绪检查(完整,检查所有依赖,如 DB/MQ/外部服务)
//
// 用法:
//   auto health = std::make_shared<sc::server::HealthEndpoint>();
//   health->addIndicator(std::make_shared<DatabaseHealthIndicator>(dbPool));
//   health->addIndicator(std::make_shared<MqHealthIndicator>(mqConnection));
//
//   // 注册到 HttpServer
//   server.get("/api/health", [health](const HttpRequest&, HttpResponse& resp) {
//       auto report = health->check();  // readiness
//       resp.setStatus(report.overall == HealthStatus::UP ? 200 : 503);
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(report.toJson());
//   });
//   server.get("/api/health/liveness", [health](const HttpRequest&, HttpResponse& resp) {
//       auto report = health->liveness();
//       resp.setStatus(report.overall == HealthStatus::UP ? 200 : 503);
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(report.toJson());
//   });

#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <QByteArray>
#include "soul/utils/json/json_helper.h"

namespace sc {
namespace server {

// ============================================================================
// HealthStatus — 健康状态枚举
// ============================================================================
enum class HealthStatus {
    UP,       ///< 服务正常
    DOWN,     ///< 服务异常
    UNKNOWN   ///< 未知状态(未检查)
};

/// @brief HealthStatus 转字符串
inline const char* toString(HealthStatus status) {
    switch (status) {
        case HealthStatus::UP:      return "UP";
        case HealthStatus::DOWN:    return "DOWN";
        case HealthStatus::UNKNOWN: return "UNKNOWN";
    }
    return "UNKNOWN";
}

// ============================================================================
// HealthDetail — 单个指示器的健康检查详情
// ============================================================================
struct HealthDetail {
    std::string    name;       ///< 指示器名称(如 "database", "mq", "network")
    HealthStatus   status = HealthStatus::UNKNOWN;
    std::string    message;    ///< 附加消息(如 "Connection pool active: 3/20")
    std::chrono::milliseconds responseTime{0};  ///< 检查耗时
};

// ============================================================================
// HealthReport — 健康检查报告
// ============================================================================
struct HealthReport {
    HealthStatus overall = HealthStatus::UNKNOWN;
    std::vector<HealthDetail> details;
    std::chrono::milliseconds totalTime{0};

    /// @brief 序列化为 JSON
    QByteArray toJson() const {
        sc::json::Json root;
        root["status"] = toString(overall);

        sc::json::Json detailsArray = sc::json::Json::array();
        for (const auto& d : details) {
            sc::json::Json detailObj;
            detailObj["name"] = d.name;
            detailObj["status"] = toString(d.status);
            if (!d.message.empty()) {
                detailObj["message"] = d.message;
            }
            detailObj["responseTimeMs"] = static_cast<qint64>(d.responseTime.count());
            detailsArray.push_back(detailObj);
        }
        root["details"] = detailsArray;
        root["totalTimeMs"] = static_cast<qint64>(totalTime.count());

        return sc::json::serialize(root);
    }
};

// ============================================================================
// IHealthIndicator — 健康检查指示器接口
// ============================================================================
//
// 每个模块实现自己的健康检查逻辑。指示器应快速返回(建议 < 100ms),
// 避免阻塞健康检查端点。
//
// @thread_safety 实现应线程安全,可并发调用 check()
class IHealthIndicator {
public:
    virtual ~IHealthIndicator() = default;

    /// @brief 执行健康检查
    /// @return 健康检查详情
    virtual HealthDetail check() const = 0;

    /// @return 指示器名称(唯一标识)
    virtual std::string name() const = 0;

    /// @return 是否为关键依赖(关键依赖 DOWN 会导致整体 DOWN)
    virtual bool isCritical() const { return true; }
};

// ============================================================================
// HealthEndpoint — 健康检查端点
// ============================================================================
//
// 聚合所有 IHealthIndicator,提供 liveness() / readiness() 两种探针。
//
// - liveness(): 仅检查关键依赖,快速返回(用于 k8s liveness probe)
// - readiness(): 检查所有依赖,完整报告(用于 k8s readiness probe)
// - check(): 等同于 readiness()
//
// @thread_safety Thread-Safe,所有方法可并发调用
class HealthEndpoint {
public:
    HealthEndpoint() = default;

    /// @brief 添加健康检查指示器
    void addIndicator(std::shared_ptr<IHealthIndicator> indicator);

    /// @brief 移除指定名称的指示器
    void removeIndicator(const std::string& name);

    /// @brief 获取所有已注册指示器的名称列表
    std::vector<std::string> indicatorNames() const;

    /// @brief 执行就绪检查(所有指示器)
    HealthReport readiness() const;

    /// @brief 执行存活检查(仅关键依赖)
    HealthReport liveness() const;

    /// @brief 等同于 readiness()
    HealthReport check() const { return readiness(); }

    /// @brief 清空所有指示器(仅用于测试)
    void clear();

private:
    HealthReport doCheck(bool criticalOnly) const;

    mutable std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<IHealthIndicator>> m_indicators;
};

// ============================================================================
// DatabaseHealthIndicator — 数据库健康检查
// ============================================================================
//
// 检查数据库连接池是否可用。通过执行简单查询(如 SELECT 1)验证连接。
//
// 用法:
//   auto dbIndicator = std::make_shared<DatabaseHealthIndicator>(
//       [](auto& db) { return db.execute("SELECT 1").isOk(); });
class DatabaseHealthIndicator : public IHealthIndicator {
public:
    using CheckFunc = std::function<bool()>;

    /// @param checkFunc 健康检查函数,返回 true 表示正常
    /// @param name      指示器名称(默认 "database")
    explicit DatabaseHealthIndicator(CheckFunc checkFunc, std::string name = "database");

    HealthDetail check() const override;
    std::string name() const override { return m_name; }
    bool isCritical() const override { return true; }

private:
    CheckFunc   m_checkFunc;
    std::string m_name;
};

// ============================================================================
// MqHealthIndicator — 消息队列健康检查
// ============================================================================
//
// 检查 MQ 连接是否可用。
class MqHealthIndicator : public IHealthIndicator {
public:
    using CheckFunc = std::function<bool()>;

    explicit MqHealthIndicator(CheckFunc checkFunc, std::string name = "mq");

    HealthDetail check() const override;
    std::string name() const override { return m_name; }
    bool isCritical() const override { return false; }  // MQ 非关键依赖

private:
    CheckFunc   m_checkFunc;
    std::string m_name;
};

// ============================================================================
// NetworkHealthIndicator — 网络健康检查
// ============================================================================
//
// 检查网络连接状态(如外部服务连通性)。
class NetworkHealthIndicator : public IHealthIndicator {
public:
    using CheckFunc = std::function<bool()>;

    explicit NetworkHealthIndicator(CheckFunc checkFunc, std::string name = "network");

    HealthDetail check() const override;
    std::string name() const override { return m_name; }
    bool isCritical() const override { return false; }

private:
    CheckFunc   m_checkFunc;
    std::string m_name;
};

// ============================================================================
// ResourcePoolHealthIndicator — 资源池健康检查
// ============================================================================
//
// 检查资源池水位(ThreadPool/ConnectionPool/DbConnectionPool 利用率)。
// 当利用率超过阈值时返回 DOWN。
class ResourcePoolHealthIndicator : public IHealthIndicator {
public:
    /// @param poolName  资源池名称(如 "ThreadPool")
    /// @param getUtilization 获取当前利用率的函数(返回 [0.0, 1.0])
    /// @param threshold 阈值(默认 0.9,超过此值返回 DOWN)
    explicit ResourcePoolHealthIndicator(std::string poolName,
                                          std::function<double()> getUtilization,
                                          double threshold = 0.9);

    HealthDetail check() const override;
    std::string name() const override { return m_poolName; }
    bool isCritical() const override { return false; }

private:
    std::string          m_poolName;
    std::function<double()> m_getUtilization;
    double               m_threshold;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_HEALTH_H