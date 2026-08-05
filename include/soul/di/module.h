#ifndef SOUL_DI_MODULE_H
#define SOUL_DI_MODULE_H

// ============================================================================
// di/module.h — DI 模块生命周期管理 + 单例注册工具
// ============================================================================
//
// 对标 Spring 的 @Configuration + @Bean 注册机制。
// Module::initialize() 初始化 Container 并注册基础类型，
// Module::shutdown() 清理所有注册。
//
// registerSingleton<T>() 将 Singleton<T> 实例注册到 DI 容器，
// wrapSingleton<T>() 将 Singleton 包装为 shared_ptr（no-op deleter）。
// ============================================================================

#include <memory>

#include "../core/singleton.h"
#include "container.h"

namespace sc {
namespace di {

/// @brief DI 模块生命周期管理
///
/// 对标 Spring 的 ApplicationContext 启动/关闭流程。
/// initialize() 注册基础类型到 Container，shutdown() 调用 Container::clear()。
class SC_DI_EXPORT Module {
public:
    /// @brief 初始化 DI 容器，注册基础类型
    static void initialize();

    /// @brief 关闭 DI 容器，清理所有注册
    static void shutdown();
};

/// @brief 将 Singleton<T> 包装为 shared_ptr（no-op deleter）
///
/// 用于将 Singleton 实例注入到需要 shared_ptr 的接口中。
/// @tparam T 单例类型（继承自 Singleton<T>）
/// @return shared_ptr<T> 指向单例实例，不管理生命周期
template<typename T>
std::shared_ptr<T> wrapSingleton() {
    return std::shared_ptr<T>(&Singleton<T>::instance(), [](T*) {});
}

/// @brief 将 Singleton<T> 注册到 DI 容器
///
/// 注册后可通过 Container::resolve<T>() 获取 shared_ptr<T>。
/// 重复注册安全忽略（AlreadyExists 错误不传播）。
/// @tparam T 单例类型（继承自 Singleton<T>）
template<typename T>
void registerSingleton() {
    auto result = Container::instance().bindInstance(&Singleton<T>::instance());
    if (!result.isOk()) {
        // Type already registered — safe to ignore for singleton registration
    }
}

} // namespace di
} // namespace sc

#endif