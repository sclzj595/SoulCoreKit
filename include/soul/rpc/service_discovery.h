#ifndef SOUL_RPC_SERVICE_DISCOVERY_H
#define SOUL_RPC_SERVICE_DISCOVERY_H

// ============================================================================
// service_discovery.h — 服务注册发现 [v2.5.0]
// ============================================================================
// 对标 Consul / Eureka / Nacos 服务发现，提供:
//   - Consul HTTP API 客户端
//   - Eureka REST API 客户端
//   - Nacos 服务发现客户端
//   - 服务健康检查 (TTL/HTTP/TCP/gRPC)
//   - 心跳保活 (Heartbeat)
//   - 服务列表缓存与刷新
//   - 负载均衡 (RoundRobin/Random/Weighted)
//   - 故障转移 (Failover)
// ============================================================================

#include <QObject>
#include <QString>
#include <QTimer>
#include <QList>
#include <QHash>
#include <QDateTime>
#include <QRandomGenerator>
#include <functional>
#include <memory>
#include <mutex>

#include "soul/core/result.h"
#include "soul/rpc/service_registry.h"

namespace sc {
namespace rpc {

// ============================================================================
// DiscoveryBackend — 服务发现后端类型
// ============================================================================
enum class DiscoveryBackend {
    Consul,
    Eureka,
    Nacos,
    InMemory
};

// ============================================================================
// DiscoveryConfig — 服务发现配置
// ============================================================================
struct DiscoveryConfig {
    DiscoveryBackend backend = DiscoveryBackend::InMemory;
    QString endpoints = "http://127.0.0.1:8500";  // Consul/Eureka/Nacos 地址
    QString serviceName = "SoulCoreKit";
    QString serviceId;                // 实例 ID (空则自动生成)
    QString host = "127.0.0.1";
    int port = 8080;
    int healthCheckIntervalMs = 10000;  // 健康检查间隔
    int heartbeatIntervalMs = 5000;     // 心跳间隔
    int deregisterAfterMs = 30000;      // 不健康后注销时间
    int refreshIntervalMs = 30000;      // 服务列表刷新间隔
    int connectTimeoutMs = 5000;        // 连接超时
    QHash<QString, QString> tags;       // 服务标签
    QHash<QString, QString> meta;       // 元数据
    bool enableHealthCheck = true;
    QString healthCheckPath = "/actuator/health";  // HTTP 健康检查路径
};

// ============================================================================
// IServiceDiscovery — 服务发现抽象接口
// ============================================================================
class IServiceDiscovery : public IServiceRegistry {
public:
    ~IServiceDiscovery() override = default;

    // === 连接管理 ===
    virtual Result<void> connect(const DiscoveryConfig& config) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;

    // === 服务实例 CRUD ===
    Result<void> registerInstance(const ServiceInstance& instance) override = 0;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override = 0;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override = 0;

    // === 健康检查 ===
    virtual Result<void> reportHealthy() = 0;
    virtual Result<void> reportUnhealthy(const QString& reason = "") = 0;

    // === 监听 ===
    using ServiceChangeCallback = std::function<void(const QList<ServiceInstance>& instances)>;
    virtual Result<void> watch(const QString& serviceName, ServiceChangeCallback callback) = 0;
    virtual Result<void> unwatch(const QString& serviceName) = 0;

    // === 状态 ===
    virtual bool isHealthy() const = 0;
    virtual QList<QString> getServiceNames() = 0;
};

// ============================================================================
// ConsulServiceDiscovery — Consul 实现
// ============================================================================
class ConsulServiceDiscovery : public QObject, public IServiceDiscovery {
    Q_OBJECT
public:
    explicit ConsulServiceDiscovery(QObject* parent = nullptr);
    ~ConsulServiceDiscovery() override;

    Result<void> connect(const DiscoveryConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    Result<void> registerInstance(const ServiceInstance& instance) override;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override;

    Result<void> reportHealthy() override;
    Result<void> reportUnhealthy(const QString& reason = "") override;

    Result<void> watch(const QString& serviceName, ServiceChangeCallback callback) override;
    Result<void> unwatch(const QString& serviceName) override;

    bool isHealthy() const override;
    QList<QString> getServiceNames() override;

private slots:
    void onHeartbeat();
    void onRefreshCache();

private:
    Result<void> sendHeartbeat();
    QByteArray syncHttpRequest(const QString& method, const QString& path,
                                const QByteArray& body = QByteArray(), int timeoutMs = 5000);

    DiscoveryConfig m_config;
    QTimer* m_heartbeatTimer = nullptr;
    QTimer* m_refreshTimer = nullptr;
    QHash<QString, QList<ServiceInstance>> m_cache;
    QHash<QString, QList<ServiceChangeCallback>> m_watchers;
    mutable std::mutex m_mutex;
    bool m_connected = false;
    bool m_healthy = false;
    QString m_checkId;
};

// ============================================================================
// EurekaServiceDiscovery — Eureka 实现
// ============================================================================
class EurekaServiceDiscovery : public QObject, public IServiceDiscovery {
    Q_OBJECT
public:
    explicit EurekaServiceDiscovery(QObject* parent = nullptr);
    ~EurekaServiceDiscovery() override;

    Result<void> connect(const DiscoveryConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    Result<void> registerInstance(const ServiceInstance& instance) override;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override;

    Result<void> reportHealthy() override;
    Result<void> reportUnhealthy(const QString& reason = "") override;

    Result<void> watch(const QString& serviceName, ServiceChangeCallback callback) override;
    Result<void> unwatch(const QString& serviceName) override;

    bool isHealthy() const override;
    QList<QString> getServiceNames() override;

private slots:
    void onHeartbeat();
    void onRefreshCache();

private:
    Result<void> sendHeartbeat();
    QByteArray syncHttpRequest(const QString& method, const QString& path,
                                const QByteArray& body = QByteArray(), int timeoutMs = 5000);

    DiscoveryConfig m_config;
    QTimer* m_heartbeatTimer = nullptr;
    QTimer* m_refreshTimer = nullptr;
    QHash<QString, QList<ServiceInstance>> m_cache;
    QHash<QString, QList<ServiceChangeCallback>> m_watchers;
    mutable std::mutex m_mutex;
    bool m_connected = false;
    bool m_healthy = false;
    QString m_instanceId;
};

// ============================================================================
// NacosServiceDiscovery — Nacos 实现
// ============================================================================
class NacosServiceDiscovery : public QObject, public IServiceDiscovery {
    Q_OBJECT
public:
    explicit NacosServiceDiscovery(QObject* parent = nullptr);
    ~NacosServiceDiscovery() override;

    Result<void> connect(const DiscoveryConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    Result<void> registerInstance(const ServiceInstance& instance) override;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override;

    Result<void> reportHealthy() override;
    Result<void> reportUnhealthy(const QString& reason = "") override;

    Result<void> watch(const QString& serviceName, ServiceChangeCallback callback) override;
    Result<void> unwatch(const QString& serviceName) override;

    bool isHealthy() const override;
    QList<QString> getServiceNames() override;

private slots:
    void onHeartbeat();
    void onRefreshCache();

private:
    Result<void> sendHeartbeat();
    QByteArray syncHttpRequest(const QString& method, const QString& path,
                                const QByteArray& body = QByteArray(), int timeoutMs = 5000);

    DiscoveryConfig m_config;
    QTimer* m_heartbeatTimer = nullptr;
    QTimer* m_refreshTimer = nullptr;
    QHash<QString, QList<ServiceInstance>> m_cache;
    QHash<QString, QList<ServiceChangeCallback>> m_watchers;
    mutable std::mutex m_mutex;
    bool m_connected = false;
    bool m_healthy = false;
    QString m_namespaceId;  // Nacos 命名空间 ID
    QString m_groupName;    // Nacos 分组名
};

// ============================================================================
// InMemoryServiceDiscovery — 内存实现 (测试/开发用)
// ============================================================================
class InMemoryServiceDiscovery : public IServiceDiscovery {
public:
    InMemoryServiceDiscovery() = default;
    ~InMemoryServiceDiscovery() override = default;

    Result<void> connect(const DiscoveryConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    Result<void> registerInstance(const ServiceInstance& instance) override;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override;

    Result<void> reportHealthy() override;
    Result<void> reportUnhealthy(const QString& reason = "") override;

    Result<void> watch(const QString& serviceName, ServiceChangeCallback callback) override;
    Result<void> unwatch(const QString& serviceName) override;

    bool isHealthy() const override;
    QList<QString> getServiceNames() override;

private:
    DiscoveryConfig m_config;
    QHash<QString, QList<ServiceInstance>> m_cache;
    QHash<QString, QList<ServiceChangeCallback>> m_watchers;
    mutable std::mutex m_mutex;
    bool m_connected = false;
    bool m_healthy = false;
};

// ============================================================================
// ServiceDiscoveryFactory — 工厂
// ============================================================================
class ServiceDiscoveryFactory {
public:
    static std::unique_ptr<IServiceDiscovery> create(DiscoveryBackend backend);
};

// ============================================================================
// WeightedLoadBalancer — 加权负载均衡
// ============================================================================
class WeightedLoadBalancer {
public:
    void setWeights(const QHash<QString, int>& weights);  // host:port → weight
    ServiceInstance select(const QList<ServiceInstance>& instances);
    void setRoundRobin();
    void setWeightedRoundRobin();
    void setLeastConnections();
    void setRandom();

private:
    int m_counter = 0;
    std::mutex m_mutex;
    QHash<QString, int> m_weights;
    enum class Strategy { RoundRobin, WeightedRoundRobin, LeastConnections, Random };
    Strategy m_strategy = Strategy::RoundRobin;
};

} // namespace rpc
} // namespace sc

#endif // SOUL_RPC_SERVICE_DISCOVERY_H