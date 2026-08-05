#ifndef SOUL_APPLICATION_APPLICATION_CONTEXT_H
#define SOUL_APPLICATION_APPLICATION_CONTEXT_H

// ============================================================================
// application_context.h — ApplicationContext (轻量级应用上下文) [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 ApplicationContext，但**不是巨型 IoC 容器**。
// 仅协调: 模块注册 → 服务生命周期 → 路由构建。
//
// 生命周期:
//   Application::createContext() → loadModules() → registerServices()
//   → initializeServices() → registerControllers() → registerRoutes()
//   → connectSignals() → Application Ready
//
// 关系: ApplicationContext 持有 ModuleRegistry/ServiceRegistry/
//       ControllerRegistry/CsRouter，不替代 DI Container。
// ============================================================================

#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "soul/core/result.h"
#include "soul/application/service_registry.h"

namespace sc::cs {
    class CsModule;
    class CsRouter;
    class CsErrorHandler;
}

namespace sc {

class ControllerRegistry;

/// @brief 轻量级应用上下文，对标 Spring 的 ApplicationContext
///
/// 协调模块注册、服务生命周期、路由构建。
/// 内部委托给 sc::di::Container 进行 DI，不替代 DI 容器。
///
/// @par 使用示例
/// @code
/// auto& ctx = ApplicationContext::instance();
/// ctx.registerModule<UserModule>();
/// ctx.registerModule<MusicModule>();
/// ctx.initialize();
///
/// // 获取服务
/// auto userService = ctx.serviceRegistry().getService<UserService>();
/// auto musicService = ctx.serviceRegistry().getService<MusicService>();
///
/// // 获取路由
/// auto& router = ctx.router();
/// router.navigate("user/list");
/// @endcode
///
/// @par 测试注入
/// @code
/// // 通过 setErrorHandler() 替换 CsErrorHandler 实现，支持单元测试 mock
/// CsErrorHandlerMock mockHandler;
/// ApplicationContext::instance().setErrorHandler(mockHandler);
/// @endcode
class ApplicationContext {
public:
    static ApplicationContext& instance();

    // === 生命周期 ===

    /// @brief 初始化应用上下文
    ///
    /// 执行顺序:
    ///   1. loadModules()         — 加载所有已注册模块
    ///   2. registerServices()    — 注册所有 Service 到 DI
    ///   3. initializeServices()  — 对标 @PostConstruct
    ///   4. registerControllers() — 注册所有 Controller
    ///   5. registerRoutes()      — 构建路由表
    ///   6. connectSignals()      — 连接信号
    Result<void> initialize();

    /// @brief 关闭应用上下文
    ///
    /// 执行顺序:
    ///   1. disconnectSignals()   — 断开信号连接
    ///   2. shutdownServices()    — 对标 @PreDestroy
    ///   3. unregisterRoutes()    — 清空路由表
    ///   4. disposeControllers()  — 释放 Controller
    ///   5. disposeServices()     — 释放 Service
    void shutdown();

    /// @brief 是否已初始化
    bool isInitialized() const { return m_initialized; }

    // === 模块管理 ===

    /// @brief 注册模块（模板版本）
    /// @tparam T CsModule 派生类
    /// @tparam Args 构造参数类型
    /// @param args 构造参数
    /// @return 模块引用
    template<typename T, typename... Args>
    T& registerModule(Args&&... args) {
        auto module = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *module;
        m_modules.push_back(std::move(module));
        return ref;
    }

    /// @brief 加载所有已注册模块
    /// 调用每个 CsModule 的 init() → onRegister() → onStart()
    Result<void> loadModules();

    // === 服务管理 ===

    /// @brief 获取 ServiceRegistry 引用
    ServiceRegistry& serviceRegistry();

    /// @brief 便捷方法: 获取指定类型的服务
    template<typename T>
    std::shared_ptr<T> getService() {
        return m_serviceRegistry->getService<T>();
    }

    // === 控制器管理 ===

    /// @brief 获取 ControllerRegistry 引用
    ControllerRegistry& controllerRegistry();

    // === 路由 ===

    /// @brief 获取 CsRouter 引用
    sc::cs::CsRouter& router();

    // === 错误处理 ===

    /// @brief 设置 CsErrorHandler（用于测试注入，必须在 initialize() 之前调用）
    /// @param errorHandler CsErrorHandler 引用
    ///
    /// 默认使用 CsErrorHandler::instance() 单例。
    /// 测试场景可通过此方法注入 mock 实现。
    void setErrorHandler(sc::cs::CsErrorHandler& errorHandler);

    // === 模块列表 ===

    /// @brief 获取所有已注册模块
    const std::vector<std::unique_ptr<sc::cs::CsModule>>& modules() const {
        return m_modules;
    }

private:
    ApplicationContext();
    ~ApplicationContext();
    ApplicationContext(const ApplicationContext&) = delete;
    ApplicationContext& operator=(const ApplicationContext&) = delete;

    std::vector<std::unique_ptr<sc::cs::CsModule>> m_modules;
    std::unique_ptr<ServiceRegistry> m_serviceRegistry;
    std::unique_ptr<ControllerRegistry> m_controllerRegistry;
    std::unique_ptr<sc::cs::CsRouter> m_csRouter;
    sc::cs::CsErrorHandler* m_errorHandler = nullptr;  ///< 非拥有指针，默认指向单例
    bool m_initialized = false;
};

} // namespace sc

#endif // SOUL_APPLICATION_APPLICATION_CONTEXT_H