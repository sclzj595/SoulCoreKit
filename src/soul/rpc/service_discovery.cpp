#include "soul/rpc/service_discovery.h"

namespace sc {
namespace rpc {

// ============================================================================
// ServiceDiscoveryBase — HTTP 后端公共实现 (Consul/Eureka/Nacos)
// ============================================================================
ServiceDiscoveryBase::ServiceDiscoveryBase(QObject* parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_refreshTimer(new QTimer(this))
    , m_networkManager(new QNetworkAccessManager(this)) {
    // 注意: 本类 override 了 IServiceDiscovery::connect(config), 会隐藏 QObject::connect,
    // 必须使用全限定 QObject::connect 以免名称遮蔽导致解析错误。
    QObject::connect(m_heartbeatTimer, &QTimer::timeout, this, &ServiceDiscoveryBase::onHeartbeat);
    QObject::connect(m_refreshTimer, &QTimer::timeout, this, &ServiceDiscoveryBase::onRefreshCache);
}

ServiceDiscoveryBase::~ServiceDiscoveryBase() {
    disconnect();
}

Result<void> ServiceDiscoveryBase::connect(const DiscoveryConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_connected = true;
    // v3.0.0: connect 只建立连接, 不隐式标记健康 (同 InMemoryServiceDiscovery)。
    m_healthy = false;
    return Result<void>::ok();
}

void ServiceDiscoveryBase::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
    m_healthy = false;
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    if (m_refreshTimer) m_refreshTimer->stop();
}

bool ServiceDiscoveryBase::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

Result<void> ServiceDiscoveryBase::registerInstance(const ServiceInstance& instance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[instance.serviceName].append(instance);
    return Result<void>::ok();
}

Result<void> ServiceDiscoveryBase::unregisterInstance(const QString& serviceName, const QString& host, int port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(serviceName);
    if (it == m_cache.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
            QString("Service not found: %1").arg(serviceName)));
    }
    QList<ServiceInstance>& instances = it.value();
    for (int i = 0; i < instances.size(); ++i) {
        if (instances[i].host == host && instances[i].port == port) {
            instances.removeAt(i);
            if (instances.isEmpty()) {
                m_cache.erase(it);
            }
            return Result<void>::ok();
        }
    }
    return Result<void>::err(Error(ErrorCode::NotFound, "Instance not found"));
}

Result<QList<ServiceInstance>> ServiceDiscoveryBase::getInstances(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Result<QList<ServiceInstance>>(m_cache.value(serviceName));
}

Result<void> ServiceDiscoveryBase::reportHealthy() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = true;
    return Result<void>::ok();
}

Result<void> ServiceDiscoveryBase::reportUnhealthy(const QString& reason) {
    Q_UNUSED(reason);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = false;
    return Result<void>::ok();
}

Result<void> ServiceDiscoveryBase::watch(const QString& serviceName, ServiceChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers[serviceName].append(std::move(callback));
    return Result<void>::ok();
}

Result<void> ServiceDiscoveryBase::unwatch(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers.remove(serviceName);
    return Result<void>::ok();
}

bool ServiceDiscoveryBase::isHealthy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_healthy;
}

QList<QString> ServiceDiscoveryBase::getServiceNames() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.keys();
}

// v2.9.3: IServiceRegistry 新增方法
Result<std::optional<ServiceInstance>> ServiceDiscoveryBase::getInstance(
    const QString& serviceName, const QString& instanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(serviceName);
    if (it == m_cache.end()) {
        return Result<std::optional<ServiceInstance>>::ok(std::nullopt);
    }
    for (const auto& inst : it.value()) {
        if (inst.instanceId == instanceId) {
            return Result<std::optional<ServiceInstance>>::ok(inst);
        }
    }
    return Result<std::optional<ServiceInstance>>::ok(std::nullopt);
}

Result<QStringList> ServiceDiscoveryBase::getAllServices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    QStringList names;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        names.append(it.key());
    }
    return Result<QStringList>::ok(names);
}

void ServiceDiscoveryBase::onHeartbeat() {
    sendHeartbeat();
}

void ServiceDiscoveryBase::onRefreshCache() {
    // stub: no-op
}

Result<void> ServiceDiscoveryBase::sendHeartbeat() {
    return Result<void>::ok();
}

QByteArray ServiceDiscoveryBase::syncHttpRequest(const QString& method, const QString& path,
                                                 const QByteArray& body, int timeoutMs) {
    if (!m_networkManager) return {};

    QUrl url(m_config.endpoints + "/" + path);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setTransferTimeout(timeoutMs);

    QNetworkReply* reply = nullptr;
    if (method == "GET") {
        reply = m_networkManager->get(request);
    } else if (method == "PUT") {
        reply = m_networkManager->put(request, body);
    } else if (method == "POST") {
        reply = m_networkManager->post(request, body);
    } else if (method == "DELETE") {
        reply = m_networkManager->deleteResource(request);
    } else {
        return {};
    }

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}

// ============================================================================
// ConsulServiceDiscovery — Consul 心跳
// ============================================================================
Result<void> ConsulServiceDiscovery::sendHeartbeat() {
    std::lock_guard<std::mutex> lock(m_mutex);
    QString checkId = m_config.serviceId.isEmpty()
        ? QString("service:%1").arg(m_config.serviceName)
        : m_config.serviceId;
    QString path = QString("v1/agent/check/pass/%1").arg(checkId);
    syncHttpRequest("PUT", path);
    return Result<void>::ok();
}

// ============================================================================
// ConsulServiceDiscovery / EurekaServiceDiscovery / NacosServiceDiscovery
// 三者无差异化逻辑，仅需继承模板扩展层 ServiceDiscoveryImpl<Traits>，
// 构造函数转发给该层即可。
// ============================================================================
ConsulServiceDiscovery::ConsulServiceDiscovery(QObject* parent)
    : ServiceDiscoveryImpl<ConsulTraits>(parent) {}

EurekaServiceDiscovery::EurekaServiceDiscovery(QObject* parent)
    : ServiceDiscoveryImpl<EurekaTraits>(parent) {}

NacosServiceDiscovery::NacosServiceDiscovery(QObject* parent)
    : ServiceDiscoveryImpl<NacosTraits>(parent) {}

// ============================================================================
// InMemoryServiceDiscovery — 内存实现 (测试/开发用)
// ============================================================================
Result<void> InMemoryServiceDiscovery::connect(const DiscoveryConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_connected = true;
    // v3.0.0: connect 只建立连接, 不隐式标记健康。
    // 健康状态需通过 reportHealthy()/reportUnhealthy() 显式上报,
    // 避免"连接成功即健康"的误报。
    m_healthy = false;
    return Result<void>::ok();
}

void InMemoryServiceDiscovery::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
    m_healthy = false;
    m_cache.clear();
    m_watchers.clear();
}

bool InMemoryServiceDiscovery::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

Result<void> InMemoryServiceDiscovery::registerInstance(const ServiceInstance& instance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[instance.serviceName].append(instance);
    // 通知 watchers
    if (auto wit = m_watchers.find(instance.serviceName); wit != m_watchers.end()) {
        for (const auto& cb : wit.value()) {
            if (cb) cb(m_cache[instance.serviceName]);
        }
    }
    return Result<void>::ok();
}

Result<void> InMemoryServiceDiscovery::unregisterInstance(const QString& serviceName, const QString& host, int port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(serviceName);
    if (it == m_cache.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
            QString("Service not found: %1").arg(serviceName)));
    }
    QList<ServiceInstance>& instances = it.value();
    for (int i = 0; i < instances.size(); ++i) {
        if (instances[i].host == host && instances[i].port == port) {
            instances.removeAt(i);
            if (instances.isEmpty()) {
                m_cache.erase(it);
            }
            return Result<void>::ok();
        }
    }
    return Result<void>::err(Error(ErrorCode::NotFound, "Instance not found"));
}

Result<QList<ServiceInstance>> InMemoryServiceDiscovery::getInstances(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Result<QList<ServiceInstance>>(m_cache.value(serviceName));
}

Result<void> InMemoryServiceDiscovery::reportHealthy() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = true;
    return Result<void>::ok();
}

Result<void> InMemoryServiceDiscovery::reportUnhealthy(const QString& reason) {
    Q_UNUSED(reason);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = false;
    return Result<void>::ok();
}

Result<void> InMemoryServiceDiscovery::watch(const QString& serviceName, ServiceChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers[serviceName].append(std::move(callback));
    // 立即通知当前状态
    if (auto it = m_cache.find(serviceName); it != m_cache.end()) {
        if (auto& back = m_watchers[serviceName].back(); back) {
            back(it.value());
        }
    }
    return Result<void>::ok();
}

Result<void> InMemoryServiceDiscovery::unwatch(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers.remove(serviceName);
    return Result<void>::ok();
}

bool InMemoryServiceDiscovery::isHealthy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_healthy;
}

QList<QString> InMemoryServiceDiscovery::getServiceNames() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.keys();
}

// v2.9.3: IServiceRegistry 新增方法
Result<std::optional<ServiceInstance>> InMemoryServiceDiscovery::getInstance(
    const QString& serviceName, const QString& instanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_cache.find(serviceName);
    if (it == m_cache.end()) {
        return Result<std::optional<ServiceInstance>>::ok(std::nullopt);
    }
    for (const auto& inst : it.value()) {
        if (inst.instanceId == instanceId) {
            return Result<std::optional<ServiceInstance>>::ok(inst);
        }
    }
    return Result<std::optional<ServiceInstance>>::ok(std::nullopt);
}

Result<QStringList> InMemoryServiceDiscovery::getAllServices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    QStringList names;
    for (auto it = m_cache.begin(); it != m_cache.end(); ++it) {
        names.append(it.key());
    }
    return Result<QStringList>::ok(names);
}

// ============================================================================
// WeightedLoadBalancer
// ============================================================================
void WeightedLoadBalancer::setWeights(const QHash<QString, int>& weights) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_weights = weights;
}

ServiceInstance WeightedLoadBalancer::select(const QList<ServiceInstance>& instances) {
    if (instances.isEmpty()) {
        return ServiceInstance();
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    int idx = 0;
    switch (m_strategy) {
    case Strategy::Random:
        idx = QRandomGenerator::global()->bounded(static_cast<int>(instances.size()));
        break;
    case Strategy::WeightedRoundRobin:
        // 加权轮询: 按权重展开实例列表后轮询
        if (!m_weights.isEmpty()) {
            QList<ServiceInstance> expanded;
            for (const auto& inst : instances) {
                QString key = QString("%1:%2").arg(inst.host).arg(inst.port);
                int weight = m_weights.value(key, 1);
                for (int w = 0; w < weight; ++w) {
                    expanded.append(inst);
                }
            }
            idx = m_counter % static_cast<int>(expanded.size());
            m_counter++;
            return expanded.at(idx);
        }
        // fallthrough to RoundRobin if no weights
        [[fallthrough]];
    case Strategy::RoundRobin:
    case Strategy::LeastConnections:  // stub: 当前无连接计数, 退化到 RoundRobin
    default:
        idx = m_counter % static_cast<int>(instances.size());
        m_counter++;
        return instances.at(idx);
    }
    return instances.at(idx);
}

void WeightedLoadBalancer::setRoundRobin() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_strategy = Strategy::RoundRobin;
}

void WeightedLoadBalancer::setWeightedRoundRobin() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_strategy = Strategy::WeightedRoundRobin;
}

void WeightedLoadBalancer::setLeastConnections() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_strategy = Strategy::LeastConnections;
}

void WeightedLoadBalancer::setRandom() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_strategy = Strategy::Random;
}

// ============================================================================
// ServiceDiscoveryFactory
// ============================================================================
std::unique_ptr<IServiceDiscovery> ServiceDiscoveryFactory::create(DiscoveryBackend backend) {
    switch (backend) {
    case DiscoveryBackend::Consul:
        return std::make_unique<ConsulServiceDiscovery>();
    case DiscoveryBackend::Eureka:
        return std::make_unique<EurekaServiceDiscovery>();
    case DiscoveryBackend::Nacos:
        return std::make_unique<NacosServiceDiscovery>();
    case DiscoveryBackend::InMemory:
    default:
        return std::make_unique<InMemoryServiceDiscovery>();
    }
}

} // namespace rpc
} // namespace sc
