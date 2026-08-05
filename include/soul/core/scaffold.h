#ifndef SOUL_CORE_SCAFFOLD_H
#define SOUL_CORE_SCAFFOLD_H

#include <memory>
#include <string>
#include "soul/core/application.h"
#include "soul/core/module.h"

namespace sc {

class ModuleRegistry;

// ============================================================================
// Scaffold — 声明式脚手架入口 [v2.0.0 重构为 Application 薄封装]
// ============================================================================
//
// 设计目标: Scaffold 是 Application 之上的声明式封装层,保持向后兼容。
// 所有模块生命周期管理已迁移至 Application,Scaffold 仅做薄转发。
//
// 用法:
//   int main(int argc, char* argv[]) {
//       sc::Scaffold scaffold(argc, argv);
//       scaffold.use(MyLoggingModule{})
//              .use(MyNetworkModule{});
//       return scaffold.run();
//   }
//
// 生命周期约定:
//   - Module 实例的生命周期由调用方管理(引用/指针),Scaffold 不拥有。
//   - Scaffold 析构时若已 run(),会自动按逆序调用已 init 成功模块的 cleanup()。
//   - 任一 Module::init() 失败时,Scaffold 会按逆序回滚已成功的模块并返回 -1。
//
// 与 sc::Application 的关系:
//   Scaffold 内部组合一个 Application 实例,所有 use()/scan()/run() 委托给
//   Application。Scaffold 是 Application 的"声明式语法糖"。
class Scaffold {
public:
    /// @brief 运行状态枚举 [v1.9.2 新增]
    enum class State {
        Uninitialized,  ///< 尚未调用 run()
        Running,        ///< 正在运行(事件循环中)
        Shutdown        ///< 已关闭
    };

    // 构造时创建 Application 实例(线程安全)。
    explicit Scaffold(int& argc, char** argv);
    ~Scaffold();

    Scaffold(const Scaffold&) = delete;
    Scaffold& operator=(const Scaffold&) = delete;
    Scaffold(Scaffold&&) = delete;
    Scaffold& operator=(Scaffold&&) = delete;

    // 声明式注册模块。返回 *this 以支持链式调用。
    // 委托给 Application::use()。
    Scaffold& use(Module& module);
    Scaffold& use(Module* module);

    // 从模块注册表扫描并注册所有模块(对标 SpringBoot 自动配置) [v1.9.1 新增]
    // 模块实例由 Scaffold 管理生命周期,scan() 返回的模块会在析构时自动清理。
    // 委托给 Application::scan()。
    Scaffold& scan(ModuleRegistry& registry);

    // 启动应用。
    //   委托给 Application::execute(),事件循环退出后自动清理。
    int run();

    // 主动关闭(通常不需要手动调用,析构函数会处理)。
    void shutdown();

    // 获取底层 Application 实例。
    Application* application() { return m_app.get(); }

    /// @brief 获取当前运行状态 [v1.9.2 新增]
    State state() const { return m_state; }

private:
    std::unique_ptr<Application> m_app;
    bool m_initialized = false;
    State m_state = State::Uninitialized;
};

} // namespace sc

#endif // SOUL_CORE_SCAFFOLD_H
