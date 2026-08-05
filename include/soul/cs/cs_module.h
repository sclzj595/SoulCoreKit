#ifndef SOUL_CS_MODULE_H
#define SOUL_CS_MODULE_H

// ============================================================================
// cs_module.h — CS 模块注册 [v2.5.0]
// ============================================================================
//
// 对标 SpringBoot 的 @Configuration + @Bean 注册。
// 将 CS 模块（Controller/Service/ViewModel）注册到 DI 容器和路由表。
//
// 关系: 依赖 sc::di::Container 和 sc::core::Module 进行模块注册。
// ============================================================================

#include <QString>
#include <QDebug>
#include <memory>
#include <typeinfo>
#include <vector>

#include "soul/cs/cs_global.h"
#include "soul/cs/cs_controller.h"
#include "soul/cs/cs_router.h"
#include "soul/cs/cs_service.h"
#include "soul/core/module.h"
#include "soul/di/container.h"
#include "soul/application/application_context.h"
#include "soul/application/service_registry.h"
#include "soul/application/controller_registry.h"

namespace sc::cs {

/// @brief CS 模块注册器（对标 Spring 的 @Configuration + @Bean）
///
/// 继承 sc::Module，在模块初始化时自动注册所有 Controller 和 Service。
/// 对标 SpringBoot 的自动配置机制。
///
/// @par 使用示例
/// @code
/// class UserModule : public CsModule {
/// public:
///     UserModule() : CsModule("User") {}
///
///     void onRegister() override {
///         // 注册 Service
///         registerService<UserService>();
///         // 注册 Controller
///         registerController<UserController>();
///     }
/// };
/// @endcode
///
/// @par 设计决策: ApplicationContext 获取方式
///
/// 当前通过 sc::ApplicationContext::instance() 单例获取 ApplicationContext。
/// 这是有意的简化——在桌面应用框架中，全局只有一个 ApplicationContext 实例，
/// 且 CsModule 的生命周期与 ApplicationContext 绑定。
/// 测试场景可通过 ApplicationContext::instance() 的全局单例隔离测试。
/// 未来 v3.0 考虑: 通过构造函数注入 ApplicationContext& 引用，进一步解耦。
class CsModule : public sc::Module {
public:
    /// @brief 构造函数
    /// @param moduleName 模块名称
    explicit CsModule(const QString& moduleName);

    /// @brief 析构函数 — 清理所有注册的 Controller
    ///
    /// 对标 Spring 的 ApplicationContext.close()，
    /// 确保所有 Controller 在模块销毁时被正确释放。
    ~CsModule() override;

    /// @brief 模块初始化（对标 Spring 的 @PostConstruct）
    /// @return 初始化结果
    Result<void> init() override;

    /// @brief 模块启动（对标 Spring 的 ContextRefreshed）
    /// @return 启动结果
    Result<void> onStart() override;

    /// @brief 模块停止（对标 Spring 的 ContextClosed）
    void onStop() override;

    /// @brief 注册 Controller 到路由表
    /// @tparam T Controller 类型
    /// @param args 构造函数参数
    /// @return Controller 指针
    template<typename T, typename... Args>
    T* registerController(Args&&... args);

    /// @brief 注册 Service 到 DI 容器
    /// @tparam T Service 类型（对标 Spring 的 @Service，默认 Singleton 作用域）
    /// @param args 构造函数参数
    /// @return Service 共享指针
    ///
    /// 创建 Service 实例并通过 Container::bindInstance() 注册到 DI 容器。
    /// 生命周期由 CsModule 持有的 shared_ptr 管理，DI 容器不拥有所有权。
    template<typename T, typename... Args>
    std::shared_ptr<T> registerService(Args&&... args);

    /// @brief 获取 CsRouter 引用
    CsRouter& router() const;

    /// @brief 获取模块名称
    QString moduleName() const { return m_moduleName; }

protected:
    /// @brief 注册回调（子类重写此方法注册 Controller/Service）
    ///
    /// 对标 SpringBoot 的 @Configuration 类中的 @Bean 方法。
    virtual void onRegister() {}

    QString m_moduleName;

    /// @brief 持有已注册 Service 的 shared_ptr，确保生命周期不早于 DI 容器
    ///
    /// 对标 Spring 的 ApplicationContext 管理 Bean 生命周期。
    /// 使用 shared_ptr<void> 利用类型擦除 + 正确析构的特性。
    /// Service 的 initialize()/shutdown() 由 ServiceRegistry 统一管理。
    std::vector<std::shared_ptr<void>> m_services;
};

// ============================================================================
// 模板实现
// ============================================================================

template<typename T, typename... Args>
T* CsModule::registerController(Args&&... args) {
    auto controller = std::make_shared<T>(std::forward<Args>(args)...);

    // 注册到 ControllerRegistry（统一管理），由 ApplicationContext::initialize() 中的
    // ControllerRegistry::registerAllRoutes() 统一注册到 CsRouter 和连接信号。
    // 对标 Spring 的 RequestMappingHandlerMapping.detectHandlerMethods()。
    auto& ctrlRegistry = sc::ApplicationContext::instance().controllerRegistry();
    // registerControllerInstance 检查重复注册，保留旧实例并返回其引用
    T& registeredRef = ctrlRegistry.registerControllerInstance<T>(std::move(controller));

    // 返回 ControllerRegistry 中实际持有的实例引用（重复注册时可能是旧实例）
    return &registeredRef;
}

template<typename T, typename... Args>
std::shared_ptr<T> CsModule::registerService(Args&&... args) {
    auto service = std::make_shared<T>(std::forward<Args>(args)...);

    auto& container = sc::di::Container::instance();

    // 对标 Spring 的 @Service — 注册为 Singleton 实例到 DI 容器
    // 使用 bindInstance 而非 bindSingleton，避免 DI 容器持有所有权导致 double-free。
    // shared_ptr 由 m_services 持有，确保生命周期覆盖整个应用运行期。
    if (!container.template isRegistered<T>()) {
        auto result = container.template bindInstance<T>(service.get());
        if (!result.isOk()) {
            // 对标 Spring 的 BeanDefinitionOverrideException 的降级处理
            qWarning() << "CsModule[" << m_moduleName << "]: Failed to register service"
                       << typeid(T).name() << "in DI container:"
                       << result.unwrapErr().message();
        }
    } else {
        qWarning() << "CsModule[" << m_moduleName << "]: Service"
                   << typeid(T).name() << "already registered in DI container, skipping.";
    }

    // 委托给 ApplicationContext 的 ServiceRegistry 进行生命周期管理
    // initialize() 由 ApplicationContext::initialize() → ServiceRegistry::initializeAll() 统一调用
    auto& svcRegistry = sc::ApplicationContext::instance().serviceRegistry();
    svcRegistry.registerServiceInstance(service);

    // 保持 shared_ptr 存活（对标 ApplicationContext 管理 Bean 生命周期）
    m_services.push_back(service);

    return service;
}

} // namespace sc::cs

#endif // SOUL_CS_MODULE_H