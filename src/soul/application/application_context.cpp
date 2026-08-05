// ============================================================================
// application_context.cpp — ApplicationContext 实现 [v2.5.0]
// ============================================================================

#include "soul/application/application_context.h"
#include "soul/application/service_registry.h"
#include "soul/application/controller_registry.h"
#include "soul/cs/cs_router.h"
#include "soul/cs/cs_error_handler.h"
#include "soul/cs/cs_module.h"
#include "soul/core/module.h"
#include "soul/di/container.h"
#include <QDebug>

namespace sc {

// ============================================================================
// 构造与析构
// ============================================================================

ApplicationContext::ApplicationContext()
    : m_serviceRegistry(std::make_unique<ServiceRegistry>())
    , m_controllerRegistry(std::make_unique<ControllerRegistry>())
    // m_csRouter 延迟到 initialize() 中创建，以支持 setErrorHandler() 注入
    , m_errorHandler(&sc::cs::CsErrorHandler::instance())
{
}

ApplicationContext::~ApplicationContext() = default;

// ============================================================================
// instance() — Meyer's Singleton
// ============================================================================

ApplicationContext& ApplicationContext::instance() {
    static ApplicationContext ctx;
    return ctx;
}

// ============================================================================
// initialize() — 完整启动流程
// ============================================================================

Result<void> ApplicationContext::initialize() {
    if (m_initialized) {
        return {}; // 幂等
    }

    qDebug() << "[ApplicationContext] Initializing...";

    // 0. 创建 CsRouter（延迟到此处以支持 setErrorHandler() 注入）
    //    对标 Spring 的 DispatcherServlet 延迟初始化。
    m_csRouter = std::make_unique<sc::cs::CsRouter>(*m_errorHandler);

    // 1. loadModules() — 加载所有已注册模块
    auto loadResult = loadModules();
    if (!loadResult.isOk()) {
        qCritical() << "[ApplicationContext] loadModules failed:"
                     << loadResult.unwrapErr().message();
        return loadResult;
    }

    // 2. registerServices() — 已在 CsModule::onRegister() 中完成
    //    （CsModule::registerService() 将服务注册到 ServiceRegistry）

    // 3. initializeServices() — 对标 @PostConstruct
    m_serviceRegistry->initializeAll();

    // 4. registerControllers() — 已在 CsModule::onRegister() 中完成
    //    （CsModule::registerController() 将控制器注册到 ControllerRegistry）

    // 5. registerRoutes() — 构建路由表
    m_controllerRegistry->registerAllRoutes(*m_csRouter);

    // 6. connectSignals() — 已在 CsRouter::registerController() 中完成
    //    （connectControllerSignals 连接了 navigationRequested 和 errorOccurred）

    m_initialized = true;
    qDebug() << "[ApplicationContext] Initialized successfully."
             << "Services:" << m_serviceRegistry->count()
             << "Controllers:" << m_controllerRegistry->count();

    return {};
}

// ============================================================================
// shutdown() — 关闭流程
// ============================================================================

void ApplicationContext::shutdown() {
    if (!m_initialized) return;

    qDebug() << "[ApplicationContext] Shutting down...";

    // 1. shutdownServices() — 对标 @PreDestroy（逆序）
    m_serviceRegistry->shutdownAll();

    // 2. disposeControllers() — 释放 Controller
    m_controllerRegistry.reset();

    // 3. unregisterRoutes() — 清空路由表
    m_csRouter.reset();

    // 4. disposeServices() — 释放 Service
    m_serviceRegistry.reset();

    // 5. disposeModules() — 释放 Module
    m_modules.clear();

    // 6. 清理 DI 容器 — 清除所有注册（bindInstance 的裸指针在 Service 销毁后失效）
    //    对标 Spring 的 ApplicationContext.close() 清理 BeanFactory。
    //    clear() 仅删除 owned=true 的单例，bindInstance 的 owned=false 不会被错误释放。
    sc::di::Container::instance().clear();

    m_initialized = false;
    qDebug() << "[ApplicationContext] Shutdown complete.";
}

// ============================================================================
// loadModules() — 加载所有已注册模块
// ============================================================================

Result<void> ApplicationContext::loadModules() {
    for (auto& module : m_modules) {
        if (!module) continue;

        qDebug() << "[ApplicationContext] Loading module:"
                 << QString::fromStdString(module->name());

        // 对标 Spring 的 @Configuration 类初始化
        auto initResult = module->init();
        if (!initResult.isOk()) {
            return initResult;
        }

        // 对标 Spring 的 ContextRefreshed
        auto startResult = module->onStart();
        if (!startResult.isOk()) {
            return startResult;
        }
    }

    return {};
}

// ============================================================================
// 访问器
// ============================================================================

ServiceRegistry& ApplicationContext::serviceRegistry() {
    return *m_serviceRegistry;
}

ControllerRegistry& ApplicationContext::controllerRegistry() {
    return *m_controllerRegistry;
}

sc::cs::CsRouter& ApplicationContext::router() {
    // CsRouter 在 initialize() 中创建，调用前需确保已初始化
    Q_ASSERT_X(m_csRouter != nullptr, "ApplicationContext::router()",
               "CsRouter not created yet. Call initialize() first.");
    return *m_csRouter;
}

// ============================================================================
// setErrorHandler() — 测试注入入口
// ============================================================================

void ApplicationContext::setErrorHandler(sc::cs::CsErrorHandler& errorHandler) {
    // 必须在 initialize() 之前调用，因为 CsRouter 在 initialize() 中创建
    if (m_initialized) {
        qWarning() << "[ApplicationContext] setErrorHandler() called after initialize(), ignored.";
        return;
    }
    m_errorHandler = &errorHandler;
}

} // namespace sc