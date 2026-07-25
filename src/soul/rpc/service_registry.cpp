#include "soul/rpc/service_registry.h"
#include <QRandomGenerator>

namespace sc {
namespace rpc {

Result<void> InMemoryServiceRegistry::registerInstance(const ServiceInstance& instance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_registry[instance.serviceName].append(instance);
    return Result<void>::ok();
}

Result<void> InMemoryServiceRegistry::unregisterInstance(const QString& serviceName, const QString& host, int port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_registry.find(serviceName);
    if (it == m_registry.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound, QString("Service not found: %1").arg(serviceName)));
    }
    QList<ServiceInstance>& instances = it.value();
    for (int i = 0; i < instances.size(); ++i) {
        if (instances[i].host == host && instances[i].port == port) {
            instances.removeAt(i);
            if (instances.isEmpty()) {
                m_registry.erase(it);
            }
            return Result<void>::ok();
        }
    }
    return Result<void>::err(Error(ErrorCode::NotFound, "Instance not found"));
}

Result<QList<ServiceInstance>> InMemoryServiceRegistry::getInstances(const QString& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Result<QList<ServiceInstance>>(m_registry.value(serviceName));
}

ServiceInstance LoadBalancer::select(const QList<ServiceInstance>& instances) {
    if (instances.isEmpty()) {
        return ServiceInstance();
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_roundRobin) {
        int idx = m_counter % instances.size();
        m_counter++;
        return instances.at(idx);
    } else {
        int idx = QRandomGenerator::global()->bounded(instances.size());
        return instances.at(idx);
    }
}

void LoadBalancer::setRoundRobin() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_roundRobin = true;
}

void LoadBalancer::setRandom() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_roundRobin = false;
}

} // namespace rpc
} // namespace sc