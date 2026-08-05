// ============================================================================
// controller_registry.cpp — ControllerRegistry 实现 [v2.5.0]
// ============================================================================

#include "soul/application/controller_registry.h"
#include "soul/cs/cs_controller.h"
#include "soul/cs/cs_router.h"
#include <QDebug>

namespace sc {

ControllerRegistry::ControllerRegistry() = default;
ControllerRegistry::~ControllerRegistry() = default;

// ============================================================================
// registerAllRoutes() — 将对标 Spring 的 HandlerMapping 注册
// ============================================================================

void ControllerRegistry::registerAllRoutes(sc::cs::CsRouter& router) {
    qDebug() << "[ControllerRegistry] Registering routes for"
             << m_controllers.size() << "controllers...";

    for (auto it = m_controllers.begin(); it != m_controllers.end(); ++it) {
        if (it.value()) {
            router.registerController(it.value().get());
        }
    }

    qDebug() << "[ControllerRegistry] All routes registered.";
}

} // namespace sc