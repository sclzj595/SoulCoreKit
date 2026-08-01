#ifndef SOUL_CORE_MODULE_REGISTRY_H
#define SOUL_CORE_MODULE_REGISTRY_H

// ============================================================================
// module_registry.h — 模块注册表(对标 SpringBoot spring.factories 自动配置)
// ============================================================================
//
// 设计目标: 提供基于注册表的自动模块发现机制,减少手动 use() 调用。
// 对标 SpringBoot 的 spring.factories / @AutoConfiguration 机制。
//
// 设计原则:
//   - 声明式注册: SC_MODULE 宏一行声明
//   - 延迟实例化: 仅在 Scaffold::scan() 时创建模块实例
//   - 与手动 use() 兼容: scan() 的模块与手动注册的模块统一管理
//   - 线程安全: 注册表读写加锁保护
//
// 用法:
//   // 在模块实现文件中声明
//   SC_MODULE(MyLoggingModule)
//   SC_MODULE(MyNetworkModule)
//
//   // 在 main() 中
//   int main(int argc, char* argv[]) {
//       sc::Scaffold scaffold(argc, argv);
//       scaffold.scan(sc::ModuleRegistry::instance());
//       return scaffold.run();
//   }

#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace sc {

class Module;

// ============================================================================
// ModuleRegistry — 模块注册表
// ============================================================================
//
// 单例,存储模块工厂函数。Scaffold::scan() 遍历注册表,
// 调用工厂创建模块实例,然后按依赖顺序初始化。
//
// @thread_safety 注册表读写加锁保护
class ModuleRegistry {
public:
    using Factory = std::function<std::unique_ptr<Module>()>;

    /// @brief 获取全局单例
    static ModuleRegistry& instance() {
        static ModuleRegistry registry;
        return registry;
    }

    ModuleRegistry(const ModuleRegistry&) = delete;
    ModuleRegistry& operator=(const ModuleRegistry&) = delete;

    /// @brief 注册模块工厂(同名模块后注册的覆盖先注册的)
    /// @param name    模块名称
    /// @param factory 工厂函数(返回 std::unique_ptr<Module>)
    void registerModule(const std::string& name, Factory factory);

    /// @brief 注销模块
    /// @param name 模块名称
    void unregisterModule(const std::string& name);

    /// @brief 获取所有已注册的模块工厂
    /// @return name -> factory 映射的快照
    std::map<std::string, Factory> factories() const;

    /// @brief 注册表是否为空
    bool empty() const;

    /// @brief 已注册模块数量
    size_t size() const;

    /// @brief 清空注册表(主要用于测试)
    void clear();

private:
    ModuleRegistry() = default;
    ~ModuleRegistry() = default;

    mutable std::mutex m_mutex;
    std::map<std::string, Factory> m_factories;
};

// ============================================================================
// 自动注册辅助类
// ============================================================================
//
// 利用静态变量初始化,在 main() 之前自动调用 ModuleRegistry::registerModule()。
// 用户侧通过 SC_MODULE 宏使用,无需直接接触此类。
template<typename ModuleType>
struct ModuleRegistrar {
    ModuleRegistrar(const std::string& name) {
        ModuleRegistry::instance().registerModule(name, []() -> std::unique_ptr<Module> {
            return std::make_unique<ModuleType>();
        });
    }
};

} // namespace sc

// ============================================================================
// SC_MODULE — 声明式模块注册宏
// ============================================================================
//
// 用法(在 .cpp 文件中):
//   SC_MODULE(MyLoggingModule)
//
// 展开为静态 ModuleRegistrar 实例,在 main() 之前自动注册模块工厂。
// 注意: 必须在 cpp 文件全局作用域使用,不能在头文件中使用(避免重复定义)。
#define SC_MODULE(ClassName) \
    static ::sc::ModuleRegistrar<ClassName> _sc_module_registrar_##ClassName(#ClassName)

#endif // SOUL_CORE_MODULE_REGISTRY_H