// ============================================================================
// service_registry.cpp — ServiceRegistry 实现 [v2.5.0]
// ============================================================================

#include "soul/application/service_registry.h"
#include <QDebug>
#include <algorithm>

namespace sc {

ServiceRegistry::ServiceRegistry() = default;
ServiceRegistry::~ServiceRegistry() = default;

// ============================================================================
// initializeAll() — 对标 Spring 的 @PostConstruct 批量调用
// ============================================================================

void ServiceRegistry::initializeAll() {
    qDebug() << "[ServiceRegistry] Initializing" << m_servicePtrs.size() << "services...";

    for (auto* service : m_servicePtrs) {
        if (service) {
            service->initialize();
        }
    }

    qDebug() << "[ServiceRegistry] All services initialized.";
}

// ============================================================================
// shutdownAll() — 对标 Spring 的 @PreDestroy 批量调用（逆序）
// ============================================================================

void ServiceRegistry::shutdownAll() {
    qDebug() << "[ServiceRegistry] Shutting down" << m_servicePtrs.size() << "services...";

    // 逆序关闭，确保依赖后创建的服务先销毁
    for (auto it = m_servicePtrs.rbegin(); it != m_servicePtrs.rend(); ++it) {
        if (*it) {
            (*it)->shutdown();
        }
    }

    m_servicePtrs.clear();
    m_services.clear();

    qDebug() << "[ServiceRegistry] All services shut down.";
}

} // namespace sc