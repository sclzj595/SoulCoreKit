#ifndef SOUL_APPLICATION_SERVICE_REGISTRY_H
#define SOUL_APPLICATION_SERVICE_REGISTRY_H

// ============================================================================
// service_registry.h — Application 层服务生命周期注册表 [v2.6.0]
// ============================================================================
//
// 注意: 与 soul/rpc/service_registry.h (RPC 服务发现注册表) 不同，
// 本文件管理服务实例的完整生命周期。
// 对标 Spring 的 BeanFactory + @PostConstruct/@PreDestroy。
//
// v2.6.0 变更:
//   - 从 ILifecycleManaged 迁移到 ILifecycle
//   - 生命周期从两阶段 (initialize/shutdown) 升级为四阶段
//   - initializeAll() → 返回 Result<void>，失败传播错误
//   - 新增 startAll() / stopAll()
//   - 内部自动管理 LifecycleState 转换
//
// 关系: ServiceRegistry 通过 ILifecycle 接口管理服务生命周期，
//       CsService 实现 ILifecycle，由 DI Container 注入。
//       不替代 DI Container，仅提供便捷的生命周期管理。
// ============================================================================

#include <QObject>
#include <QString>
#include <QHash>
#include <QDebug>
#include <memory>
#include <vector>
#include <typeindex>

#include "soul/core/result.h"
#include "soul/core/lifecycle.h"

namespace sc {

/// @brief 服务注册表，管理服务实例的完整生命周期
///
/// 对标 Spring 的 BeanFactory，通过 ILifecycle 接口提供:
///   - registerService<T>()   — 注册服务实例
///   - getService<T>()        — 获取服务实例
///   - initializeAll()        — 对标 @PostConstruct
///   - startAll()             — 对标 ContextRefreshed
///   - stopAll()              — 对标 ContextClosed
///   - shutdownAll()          — 对标 @PreDestroy
///
/// @par 使用示例
/// @code
/// auto& registry = ApplicationContext::instance().serviceRegistry();
///
/// // 注册服务
/// auto userService = registry.registerService<UserService>(repo);
///
/// // 获取服务
/// auto svc = registry.getService<UserService>();
///
/// // 完整生命周期
/// registry.initializeAll();
/// registry.startAll();
/// // ... 运行中 ...
/// registry.stopAll();
/// registry.shutdownAll();
/// @endcode
class ServiceRegistry {
public:
    ServiceRegistry();
    ~ServiceRegistry();

    // === 注册 ===

    /// @brief 注册服务实例
    /// @tparam T 实现 ILifecycle 的服务类型
    /// @tparam Args 构造参数类型
    /// @param args 构造参数
    /// @return 服务 shared_ptr
    ///
    /// 服务自动注册到 sc::di::Container 并调用 initialize()。
    template<typename T, typename... Args>
    std::shared_ptr<T> registerService(Args&&... args) {
        auto service = std::make_shared<T>(std::forward<Args>(args)...);
        return registerServiceInstance(std::move(service));
    }

    /// @brief 注册已创建的服务实例
    /// @tparam T 实现 ILifecycle 的服务类型
    /// @param service 服务 shared_ptr
    /// @return 已注册的服务 shared_ptr（重复注册时返回已存在的实例）
    template<typename T>
    std::shared_ptr<T> registerServiceInstance(std::shared_ptr<T> service) {
        if (!service) {
            return nullptr;
        }

        auto index = std::type_index(typeid(T));
        if (m_services.contains(index)) {
            qWarning() << "ServiceRegistry: Service" << typeid(T).name()
                       << "already registered. Returning existing instance.";
            return std::static_pointer_cast<T>(m_services.value(index));
        }

        // 存储 ILifecycle* 指针用于生命周期回调
        auto* lifecycle = static_cast<ILifecycle*>(service.get());
        m_lifecyclePtrs.push_back(lifecycle);
        m_services.insert(index, service);

        return service;
    }

    // === 获取 ===

    /// @brief 获取服务实例
    /// @tparam T 实现 ILifecycle 的服务类型
    /// @return 服务 shared_ptr，未注册时返回 nullptr
    template<typename T>
    std::shared_ptr<T> getService() {
        auto index = std::type_index(typeid(T));
        auto it = m_services.find(index);
        if (it != m_services.end()) {
            return std::static_pointer_cast<T>(it.value());
        }
        return nullptr;
    }

    /// @brief 检查服务是否已注册
    template<typename T>
    bool isRegistered() const {
        return m_services.contains(std::type_index(typeid(T)));
    }

    // === 生命周期 (v2.6.0: 四阶段) ===

    /// @brief 初始化所有已注册服务（对标 @PostConstruct）
    /// @return 首个失败的错误，或 Ok
    Result<void> initializeAll();

    /// @brief 启动所有已注册服务（对标 ContextRefreshedEvent）
    /// @return 首个失败的错误，或 Ok
    Result<void> startAll();

    /// @brief 停止所有已注册服务（对标 ContextClosedEvent）
    /// 逆序执行，保证每个服务都执行 stop()
    void stopAll();

    /// @brief 关闭所有已注册服务（对标 @PreDestroy）
    /// 逆序执行，保证每个服务都执行 shutdown()
    void shutdownAll();

    /// @brief 获取已注册服务数量
    size_t count() const { return m_services.size(); }

private:
    // 使用 type_index 作为 key，shared_ptr<void> 类型擦除存储
    QHash<std::type_index, std::shared_ptr<void>> m_services;

    // ILifecycle* 裸指针数组，用于生命周期回调
    // 生命周期由 m_services 中的 shared_ptr 管理，此处仅为便利访问
    std::vector<ILifecycle*> m_lifecyclePtrs;
};

} // namespace sc

#endif // SOUL_APPLICATION_SERVICE_REGISTRY_H
