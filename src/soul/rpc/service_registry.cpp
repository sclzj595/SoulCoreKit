// ============================================================================
// service_registry.cpp — 服务注册实现 [v2.9.3 增强 / v3.0.0]
// ============================================================================

#include "soul/rpc/service_registry.h"

namespace sc {
namespace rpc {

// ============================================================================
// InMemoryServiceRegistry
// ============================================================================

Result<void> InMemoryServiceRegistry::registerInstance(const ServiceInstance& instance) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_registry[instance.serviceName].append(instance);
    return Result<void>::ok();
}

Result<void> InMemoryServiceRegistry::unregisterInstance(
    const QString& serviceName, const QString& host, int port) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_registry.find(serviceName);
    if (it == m_registry.end()) {
        return Result<void>::err(
            Error(ErrorCode::NotFound, QString("Service not found: %1").arg(serviceName)));
    }
    QList<ServiceInstance>& instances = it.value();
    for (int i = 0, sz = static_cast<int>(instances.size()); i < sz; ++i) {
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
    return Result<QList<ServiceInstance>>::ok(m_registry.value(serviceName));
}

// v2.9.3 新增
Result<std::optional<ServiceInstance>> InMemoryServiceRegistry::getInstance(
    const QString& serviceName, const QString& instanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_registry.find(serviceName);
    if (it == m_registry.end()) {
        return Result<std::optional<ServiceInstance>>::ok(std::nullopt);
    }
    for (const auto& inst : it.value()) {
        if (inst.instanceId == instanceId) {
            return Result<std::optional<ServiceInstance>>::ok(inst);
        }
    }
    return Result<std::optional<ServiceInstance>>::ok(std::nullopt);
}

Result<QStringList> InMemoryServiceRegistry::getAllServices() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return Result<QStringList>::ok(QStringList(m_registry.keys().begin(), m_registry.keys().end()));
}

Result<void> InMemoryServiceRegistry::unregisterById(
    const QString& serviceName, const QString& instanceId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_registry.find(serviceName);
    if (it == m_registry.end()) {
        return Result<void>::err(
            Error(ErrorCode::NotFound, QString("Service not found: %1").arg(serviceName)));
    }
    QList<ServiceInstance>& instances = it.value();
    for (int i = 0, sz = static_cast<int>(instances.size()); i < sz; ++i) {
        if (instances[i].instanceId == instanceId) {
            instances.removeAt(i);
            if (instances.isEmpty()) {
                m_registry.erase(it);
            }
            return Result<void>::ok();
        }
    }
    return Result<void>::err(Error(ErrorCode::NotFound, "Instance not found"));
}

} // namespace rpc
} // namespace sc
