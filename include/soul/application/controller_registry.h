#ifndef SOUL_APPLICATION_CONTROLLER_REGISTRY_H
#define SOUL_APPLICATION_CONTROLLER_REGISTRY_H

// ============================================================================
// controller_registry.h — ControllerRegistry (控制器注册表) [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 RequestMappingHandlerMapping。
// 管理 CsController 实例的注册、获取和路由注册。
//
// 关系: ControllerRegistry 持有所有 CsController，
//       通过 CsRouter 注册路由表。
// ============================================================================

#include <QObject>
#include <QString>
#include <QHash>
#include <QDebug>
#include <memory>
#include <vector>
#include <typeindex>

#include "soul/core/result.h"

namespace sc::cs {
    class CsController;
    class CsRouter;
}

namespace sc {

/// @brief 控制器注册表，管理 CsController 实例的生命周期
///
/// 对标 Spring 的 HandlerMapping，提供:
///   - registerController<T>()   — 注册控制器实例
///   - getController<T>()        — 获取控制器实例
///   - registerAllRoutes()       — 将所有控制器路由注册到 CsRouter
///
/// @par 使用示例
/// @code
/// auto& registry = ApplicationContext::instance().controllerRegistry();
///
/// // 注册控制器
/// auto& ctrl = registry.registerController<UserController>(userService);
///
/// // 注册所有路由
/// registry.registerAllRoutes(router);
/// @endcode
class ControllerRegistry {
public:
    ControllerRegistry();
    ~ControllerRegistry();

    // === 注册 ===

    /// @brief 注册控制器实例（模板版本）
    /// @tparam T CsController 派生类
    /// @tparam Args 构造参数类型
    /// @param args 构造参数
    /// @return 控制器引用
    ///
    /// 重复注册时保留旧实例，避免已有引用悬空。
    template<typename T, typename... Args>
    T& registerController(Args&&... args) {
        auto controller = std::make_shared<T>(std::forward<Args>(args)...);
        T& ref = *controller;

        auto index = std::type_index(typeid(T));
        if (m_controllers.contains(index)) {
            qWarning() << "ControllerRegistry: Controller" << typeid(T).name()
                       << "already registered. Keeping existing instance.";
            return *static_cast<T*>(m_controllers.value(index).get());
        }

        m_controllers.insert(index, std::move(controller));
        return ref;
    }

    /// @brief 注册已创建的控制器实例（shared_ptr 共享所有权）
    /// @tparam T CsController 派生类
    /// @param controller 控制器 shared_ptr
    /// @return 控制器引用
    ///
    /// 供 CsModule::registerController 调用，统一管理 Controller 生命周期。
    /// 重复注册时保留旧实例。
    template<typename T>
    T& registerControllerInstance(std::shared_ptr<T> controller) {
        T& ref = *controller;

        auto index = std::type_index(typeid(T));
        if (m_controllers.contains(index)) {
            qWarning() << "ControllerRegistry: Controller" << typeid(T).name()
                       << "already registered. Keeping existing instance.";
            return *static_cast<T*>(m_controllers.value(index).get());
        }

        m_controllers.insert(index, std::move(controller));
        return ref;
    }

    // === 获取 ===

    /// @brief 获取控制器实例
    /// @tparam T CsController 派生类
    /// @return 控制器指针，未注册时返回 nullptr
    template<typename T>
    T* getController() {
        auto index = std::type_index(typeid(T));
        auto it = m_controllers.find(index);
        if (it != m_controllers.end()) {
            return static_cast<T*>(it.value().get());
        }
        return nullptr;
    }

    /// @brief 检查控制器是否已注册
    template<typename T>
    bool isRegistered() const {
        return m_controllers.contains(std::type_index(typeid(T)));
    }

    // === 路由注册 ===

    /// @brief 将所有已注册控制器的路由注册到 CsRouter
    /// @param router CsRouter 引用
    ///
    /// 遍历所有 Controller，调用 router.registerController()。
    /// 对标 Spring 的 AbstractHandlerMethodMapping.detectHandlerMethods()。
    void registerAllRoutes(sc::cs::CsRouter& router);

    /// @brief 获取已注册控制器数量
    size_t count() const { return m_controllers.size(); }

private:
    // 使用 shared_ptr 管理 Controller 生命周期
    // key 为 type_index，value 为 shared_ptr<CsController>
    // (QHash 不支持 unique_ptr 值类型，与 ServiceRegistry 保持一致使用 shared_ptr)
    QHash<std::type_index, std::shared_ptr<sc::cs::CsController>> m_controllers;
};

} // namespace sc

#endif // SOUL_APPLICATION_CONTROLLER_REGISTRY_H