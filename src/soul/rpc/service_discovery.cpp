#include "soul/rpc/service_discovery.h"

namespace sc {
namespace rpc {

// ============================================================================
// ConsulServiceDiscovery
// ============================================================================
ConsulServiceDiscovery::ConsulServiceDiscovery(QObject* parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_refreshTimer(new QTimer(this)) {
    connect(m_heartbeatTimer, &QTimer::timeout, this, &ConsulServiceDiscovery::onHeartbeat);
    connect(m_refreshTimer, &QTimer::timeout, this, &ConsulServiceDiscovery::onRefreshCache);
}

ConsulServiceDiscovery::~ConsulServiceDiscovery() {
    disconnect();
}

Result<void> ConsulServiceDiscovery::connect(const DiscoveryConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_connected = true;
    m_healthy = true;
    return Result<void>::ok();
}

void ConsulServiceDiscovery::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
    m_healthy = false;
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    if (m_refreshTimer) m_refreshTimer->stop();
}

bool ConsulServiceDiscovery::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

Result<void> ConsulServiceDiscovery::registerInstance(const ServiceInstance& instance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[instance.serviceName].append(instance);
    return Result<void>::ok();
}

Result<void> ConsulServiceDiscovery::unregisterInstance(const QString& serviceName, const QString& host, int port) {
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

Result<QList<ServiceInstance>> ConsulServiceDiscovery::getInstances(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Result<QList<ServiceInstance>>(m_cache.value(serviceName));
}

Result<void> ConsulServiceDiscovery::reportHealthy() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = true;
    return Result<void>::ok();
}

Result<void> ConsulServiceDiscovery::reportUnhealthy(const QString& reason) {
    Q_UNUSED(reason);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = false;
    return Result<void>::ok();
}

Result<void> ConsulServiceDiscovery::watch(const QString& serviceName, ServiceChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers[serviceName].append(std::move(callback));
    return Result<void>::ok();
}

Result<void> ConsulServiceDiscovery::unwatch(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers.remove(serviceName);
    return Result<void>::ok();
}

bool ConsulServiceDiscovery::isHealthy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_healthy;
}

QList<QString> ConsulServiceDiscovery::getServiceNames() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.keys();
}

void ConsulServiceDiscovery::onHeartbeat() {
    sendHeartbeat();
}

void ConsulServiceDiscovery::onRefreshCache() {
    // stub: no-op
}

Result<void> ConsulServiceDiscovery::sendHeartbeat() {
    return Result<void>::ok();
}

QByteArray ConsulServiceDiscovery::syncHttpRequest(const QString& method, const QString& path,
                                                    const QByteArray& body, int timeoutMs) {
    Q_UNUSED(method);
    Q_UNUSED(path);
    Q_UNUSED(body);
    Q_UNUSED(timeoutMs);
    return QByteArray();
}

// ============================================================================
// EurekaServiceDiscovery
// ============================================================================
EurekaServiceDiscovery::EurekaServiceDiscovery(QObject* parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_refreshTimer(new QTimer(this)) {
    connect(m_heartbeatTimer, &QTimer::timeout, this, &EurekaServiceDiscovery::onHeartbeat);
    connect(m_refreshTimer, &QTimer::timeout, this, &EurekaServiceDiscovery::onRefreshCache);
}

EurekaServiceDiscovery::~EurekaServiceDiscovery() {
    disconnect();
}

Result<void> EurekaServiceDiscovery::connect(const DiscoveryConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_connected = true;
    m_healthy = true;
    return Result<void>::ok();
}

void EurekaServiceDiscovery::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
    m_healthy = false;
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    if (m_refreshTimer) m_refreshTimer->stop();
}

bool EurekaServiceDiscovery::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

Result<void> EurekaServiceDiscovery::registerInstance(const ServiceInstance& instance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[instance.serviceName].append(instance);
    return Result<void>::ok();
}

Result<void> EurekaServiceDiscovery::unregisterInstance(const QString& serviceName, const QString& host, int port) {
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

Result<QList<ServiceInstance>> EurekaServiceDiscovery::getInstances(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Result<QList<ServiceInstance>>(m_cache.value(serviceName));
}

Result<void> EurekaServiceDiscovery::reportHealthy() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = true;
    return Result<void>::ok();
}

Result<void> EurekaServiceDiscovery::reportUnhealthy(const QString& reason) {
    Q_UNUSED(reason);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = false;
    return Result<void>::ok();
}

Result<void> EurekaServiceDiscovery::watch(const QString& serviceName, ServiceChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers[serviceName].append(std::move(callback));
    return Result<void>::ok();
}

Result<void> EurekaServiceDiscovery::unwatch(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers.remove(serviceName);
    return Result<void>::ok();
}

bool EurekaServiceDiscovery::isHealthy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_healthy;
}

QList<QString> EurekaServiceDiscovery::getServiceNames() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.keys();
}

void EurekaServiceDiscovery::onHeartbeat() {
    sendHeartbeat();
}

void EurekaServiceDiscovery::onRefreshCache() {
    // stub: no-op
}

Result<void> EurekaServiceDiscovery::sendHeartbeat() {
    return Result<void>::ok();
}

QByteArray EurekaServiceDiscovery::syncHttpRequest(const QString& method, const QString& path,
                                                    const QByteArray& body, int timeoutMs) {
    Q_UNUSED(method);
    Q_UNUSED(path);
    Q_UNUSED(body);
    Q_UNUSED(timeoutMs);
    return QByteArray();
}

// ============================================================================
// NacosServiceDiscovery
// ============================================================================
NacosServiceDiscovery::NacosServiceDiscovery(QObject* parent)
    : QObject(parent)
    , m_heartbeatTimer(new QTimer(this))
    , m_refreshTimer(new QTimer(this)) {
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NacosServiceDiscovery::onHeartbeat);
    connect(m_refreshTimer, &QTimer::timeout, this, &NacosServiceDiscovery::onRefreshCache);
}

NacosServiceDiscovery::~NacosServiceDiscovery() {
    disconnect();
}

Result<void> NacosServiceDiscovery::connect(const DiscoveryConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_connected = true;
    m_healthy = true;
    return Result<void>::ok();
}

void NacosServiceDiscovery::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
    m_healthy = false;
    if (m_heartbeatTimer) m_heartbeatTimer->stop();
    if (m_refreshTimer) m_refreshTimer->stop();
}

bool NacosServiceDiscovery::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

Result<void> NacosServiceDiscovery::registerInstance(const ServiceInstance& instance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_cache[instance.serviceName].append(instance);
    return Result<void>::ok();
}

Result<void> NacosServiceDiscovery::unregisterInstance(const QString& serviceName, const QString& host, int port) {
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

Result<QList<ServiceInstance>> NacosServiceDiscovery::getInstances(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Result<QList<ServiceInstance>>(m_cache.value(serviceName));
}

Result<void> NacosServiceDiscovery::reportHealthy() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = true;
    return Result<void>::ok();
}

Result<void> NacosServiceDiscovery::reportUnhealthy(const QString& reason) {
    Q_UNUSED(reason);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_healthy = false;
    return Result<void>::ok();
}

Result<void> NacosServiceDiscovery::watch(const QString& serviceName, ServiceChangeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers[serviceName].append(std::move(callback));
    return Result<void>::ok();
}

Result<void> NacosServiceDiscovery::unwatch(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watchers.remove(serviceName);
    return Result<void>::ok();
}

bool NacosServiceDiscovery::isHealthy() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_healthy;
}

QList<QString> NacosServiceDiscovery::getServiceNames() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_cache.keys();
}

void NacosServiceDiscovery::onHeartbeat() {
    sendHeartbeat();
}

void NacosServiceDiscovery::onRefreshCache() {
    // stub: no-op
}

Result<void> NacosServiceDiscovery::sendHeartbeat() {
    return Result<void>::ok();
}

QByteArray NacosServiceDiscovery::syncHttpRequest(const QString& method, const QString& path,
                                                   const QByteArray& body, int timeoutMs) {
    Q_UNUSED(method);
    Q_UNUSED(path);
    Q_UNUSED(body);
    Q_UNUSED(timeoutMs);
    return QByteArray();
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
    int idx = m_counter % static_cast<int>(instances.size());
    m_counter++;
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
        return nullptr;
    }
}

} // namespace rpc
} // namespace sc