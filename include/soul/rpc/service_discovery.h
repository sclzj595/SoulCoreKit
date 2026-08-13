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
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QEventLoop>
#include <QUrl>
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
// IServiceDiscovery — 服务发现抽象接口 [v2.9.3 增强]
// ============================================================================
//
// 生命周期 (v2.9.3 明确):
//   Created → connect() → registerInstance() → Running
//     → unregisterInstance() → disconnect() → Destroyed
//
// 异常路径:
//   Running → backend lost → reportUnhealthy() → reconnect → reportHealthy()
//
// 线程安全 (v2.9.3 明确):
//   - register/unregister/discover: 线程安全
//   - watch callback: 在内部线程执行，禁止重入注册/注销操作
//   - disconnect: 阻塞等待所有回调完成
//   - shutdown 幂等: 重复调用安全
//
// Health 集成:
//   Adapter 可提供 IHealthIndicator，通过反向注册接入 HealthAggregator。
//   IServiceDiscovery 不直接依赖 Health 模块。
class IServiceDiscovery : public IServiceRegistry {
public:
    ~IServiceDiscovery() override = default;

    // === 连接管理 (生命周期) ===
    /// @brief 连接到服务发现后端
    virtual Result<void> connect(const DiscoveryConfig& config) = 0;

    /// @brief 断开连接 (阻塞等待回调完成，幂等)
    virtual void disconnect() = 0;

    /// @brief 是否已连接
    virtual bool isConnected() const = 0;

    // === 服务实例 CRUD ===
    Result<void> registerInstance(const ServiceInstance& instance) override = 0;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override = 0;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override = 0;

    // v2.9.3: IServiceRegistry 新增方法
    Result<std::optional<ServiceInstance>> getInstance(const QString& serviceName,
                                                        const QString& instanceId) override = 0;
    Result<QStringList> getAllServices() override = 0;

    // === 健康检查 ===
    /// @brief 报告本实例健康
    virtual Result<void> reportHealthy() = 0;

    /// @brief 报告本实例不健康
    virtual Result<void> reportUnhealthy(const QString& reason = "") = 0;

    // === 监听 ===
    /// @brief 监听服务实例变化
    /// @note callback 在内部线程执行，禁止在回调中 register/unregister
    using ServiceChangeCallback = std::function<void(const QList<ServiceInstance>& instances)>;
    virtual Result<void> watch(const QString& serviceName, ServiceChangeCallback callback) = 0;
    virtual Result<void> unwatch(const QString& serviceName) = 0;

    // === 状态 ===
    virtual bool isHealthy() const = 0;

    /// @brief 获取所有服务名称 [v2.9.3: 重命名自 getServiceNames]
    virtual QList<QString> getServiceNames() = 0;

};

// ============================================================================
// ServiceDiscoveryBase — HTTP 后端公共基类 (Consul/Eureka/Nacos)
// ============================================================================
// 三个 HTTP 后端的通用逻辑 (缓存 / watcher / 状态 / 定时器) 全部收敛到本基类，
// 各后端类仅保留类型标识，避免三份重复实现。
class ServiceDiscoveryBase : public QObject, public IServiceDiscovery {
    Q_OBJECT
public:
    explicit ServiceDiscoveryBase(QObject* parent = nullptr);
    ~ServiceDiscoveryBase() override;

    Result<void> connect(const DiscoveryConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    Result<void> registerInstance(const ServiceInstance& instance) override;
    Result<void> unregisterInstance(const QString& serviceName, const QString& host, int port) override;
    Result<QList<ServiceInstance>> getInstances(const QString& serviceName) override;

    // v2.9.3: IServiceRegistry 新增方法
    Result<std::optional<ServiceInstance>> getInstance(const QString& serviceName,
                                                        const QString& instanceId) override;
    Result<QStringList> getAllServices() override;

    Result<void> reportHealthy() override;
    Result<void> reportUnhealthy(const QString& reason = "") override;

    Result<void> watch(const QString& serviceName, ServiceChangeCallback callback) override;
    Result<void> unwatch(const QString& serviceName) override;

    bool isHealthy() const override;
    QList<QString> getServiceNames() override;

protected:
    // 供子类覆盖的后端差异化行为 (默认实现为通用 HTTP 同步请求)
    virtual Result<void> sendHeartbeat();
    virtual QByteArray syncHttpRequest(const QString& method, const QString& path,
                                       const QByteArray& body = QByteArray(),
                                       int timeoutMs = 5000);

    DiscoveryConfig m_config;
    QTimer* m_heartbeatTimer = nullptr;
    QTimer* m_refreshTimer = nullptr;
    QNetworkAccessManager* m_networkManager = nullptr;
    QHash<QString, QList<ServiceInstance>> m_cache;
    QHash<QString, QList<ServiceChangeCallback>> m_watchers;
    mutable std::mutex m_mutex;
    bool m_connected = false;
    bool m_healthy = false;

private slots:
    void onHeartbeat();
    void onRefreshCache();
};

// ============================================================================
// 后端 Traits — 类型化扩展点
// ============================================================================
// 每个 HTTP 后端的差异化类型与解析逻辑收敛到对应的 Traits 结构。
// 当前三个后端均为占位空结构 (差异化成员在早期版本中全是死代码);
// 后续接入真实后端时, 只需向 Traits 添加字段/方法, 复用 ServiceDiscoveryImpl
// 的通用逻辑即可, 无需改动模板本体。
struct ConsulTraits {};
struct EurekaTraits {};
struct NacosTraits {};

// ============================================================================
// ServiceDiscoveryImpl — 模板参数化扩展层
// ============================================================================
// 注意: Qt 的 moc 不支持模板类, 故本层不带 Q_OBJECT, 通用逻辑与信号槽
// (onHeartbeat/onRefreshCache) 全部保留在非模板的 ServiceDiscoveryBase 中。
// 本层仅提供 BackendTraits 泛型, 供未来各后端注入差异化实现。
template <typename BackendTraits>
class ServiceDiscoveryImpl : public ServiceDiscoveryBase {
public:
    using ServiceDiscoveryBase::ServiceDiscoveryBase;  // 继承构造
    using Traits = BackendTraits;
};

// ============================================================================
// ConsulServiceDiscovery — Consul 实现
// ============================================================================
class ConsulServiceDiscovery : public ServiceDiscoveryImpl<ConsulTraits> {
    Q_OBJECT
public:
    explicit ConsulServiceDiscovery(QObject* parent = nullptr);

protected:
    // Consul Agent Check TTL heartbeat: PUT /v1/agent/check/pass/{checkId}
    Result<void> sendHeartbeat() override;
};

// ============================================================================
// EurekaServiceDiscovery — Eureka 实现
// ============================================================================
class EurekaServiceDiscovery : public ServiceDiscoveryImpl<EurekaTraits> {
    Q_OBJECT
public:
    explicit EurekaServiceDiscovery(QObject* parent = nullptr);
};

// ============================================================================
// NacosServiceDiscovery — Nacos 实现
// ============================================================================
class NacosServiceDiscovery : public ServiceDiscoveryImpl<NacosTraits> {
    Q_OBJECT
public:
    explicit NacosServiceDiscovery(QObject* parent = nullptr);
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

    // v2.9.3: IServiceRegistry 新增方法
    Result<std::optional<ServiceInstance>> getInstance(const QString& serviceName,
                                                        const QString& instanceId) override;
    Result<QStringList> getAllServices() override;

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