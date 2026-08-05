// ============================================================================
// cs_module.cpp — CS 模块注册实现 [v2.5.0]
// ============================================================================

#include "soul/cs/cs_module.h"
#include "soul/cs/cs_router.h"
#include "soul/cs/cs_service.h"

namespace sc::cs {

CsModule::CsModule(const QString& moduleName)
    : sc::Module(moduleName.toStdString())
    , m_moduleName(moduleName)
{
}

CsModule::~CsModule() {
    // 对标 Spring 的 ApplicationContext.close() — 清理所有 Bean
    // 先停止模块，再释放 Service
    onStop();

    // Service 的 shutdown() 由 ApplicationContext::shutdown() →
    // ServiceRegistry::shutdownAll() 统一调用，此处不重复调用。
    // Controller 由 ControllerRegistry 统一管理生命周期。

    // shared_ptr 自动析构释放 Service（当引用计数归零时）
    m_services.clear();
}

Result<void> CsModule::init() {
    onRegister();
    return {};
}

Result<void> CsModule::onStart() {
    return {};
}

void CsModule::onStop() {
    // 对标 Spring 的 @PreDestroy — 子类可重写此方法执行清理
    // Service 的 shutdown() 在析构函数中统一调用，确保在 onStop 之后
}

CsRouter& CsModule::router() const {
    return sc::ApplicationContext::instance().router();
}

} // namespace sc::cs