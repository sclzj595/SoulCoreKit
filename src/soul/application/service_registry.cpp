// ============================================================================
// service_registry.cpp — ServiceRegistry 实现 [v2.6.0]
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

Result<void> ServiceRegistry::initializeAll() {
    qDebug() << "[ServiceRegistry] Initializing" << m_lifecyclePtrs.size() << "services...";

    for (size_t i = 0; i < m_lifecyclePtrs.size(); ++i) {
        auto* service = m_lifecyclePtrs[i];
        if (!service) continue;

        auto result = service->initialize();
        if (result.isErr()) {
            qWarning() << "[ServiceRegistry] Service initialize failed at index"
                       << i << ":" << result.unwrapErr().message()
                       << "— rolling back previous services";

            // 逆序 shutdown 已初始化的服务
            for (size_t j = i; j > 0; --j) {
                auto* prev = m_lifecyclePtrs[j - 1];
                if (prev) {
                    prev->shutdown();
                }
            }
            return result;
        }
    }

    qDebug() << "[ServiceRegistry] All services initialized.";
    return {};
}

// ============================================================================
// startAll() — 对标 Spring 的 ContextRefreshedEvent 批量调用
// ============================================================================

Result<void> ServiceRegistry::startAll() {
    qDebug() << "[ServiceRegistry] Starting" << m_lifecyclePtrs.size() << "services...";

    for (size_t i = 0; i < m_lifecyclePtrs.size(); ++i) {
        auto* service = m_lifecyclePtrs[i];
        if (!service) continue;

        auto result = service->start();
        if (result.isErr()) {
            qWarning() << "[ServiceRegistry] Service start failed at index"
                       << i << ":" << result.unwrapErr().message()
                       << "— stopping and shutting down previous services";

            // 逆序 stop + shutdown 已启动的服务
            for (size_t j = i; j > 0; --j) {
                auto* prev = m_lifecyclePtrs[j - 1];
                if (prev) {
                    prev->stop();
                }
            }
            for (size_t j = i; j > 0; --j) {
                auto* prev = m_lifecyclePtrs[j - 1];
                if (prev) {
                    prev->shutdown();
                }
            }
            return result;
        }
    }

    qDebug() << "[ServiceRegistry] All services started.";
    return {};
}

// ============================================================================
// stopAll() — 对标 Spring 的 ContextClosedEvent 批量调用（逆序）
// ============================================================================

void ServiceRegistry::stopAll() {
    qDebug() << "[ServiceRegistry] Stopping" << m_lifecyclePtrs.size() << "services...";

    // 逆序停止，确保依赖后创建的服务先停止
    for (auto it = m_lifecyclePtrs.rbegin(); it != m_lifecyclePtrs.rend(); ++it) {
        if (*it) {
            (*it)->stop();
        }
    }

    qDebug() << "[ServiceRegistry] All services stopped.";
}

// ============================================================================
// shutdownAll() — 对标 Spring 的 @PreDestroy 批量调用（逆序）
// ============================================================================

void ServiceRegistry::shutdownAll() {
    qDebug() << "[ServiceRegistry] Shutting down" << m_lifecyclePtrs.size() << "services...";

    // 逆序关闭，确保依赖后创建的服务先销毁
    for (auto it = m_lifecyclePtrs.rbegin(); it != m_lifecyclePtrs.rend(); ++it) {
        if (*it) {
            (*it)->shutdown();
        }
    }

    m_lifecyclePtrs.clear();
    m_services.clear();

    qDebug() << "[ServiceRegistry] All services shut down.";
}

} // namespace sc
