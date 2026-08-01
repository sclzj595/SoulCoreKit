#ifndef SOUL_CORE_SCAFFOLD_H
#define SOUL_CORE_SCAFFOLD_H

#include <memory>
#include <vector>
#include <string>
#include "soul/core/application.h"
#include "soul/core/module.h"
#include "soul/core/result.h"

namespace sc {

class ModuleRegistry;

// ============================================================================
// Scaffold — 声明式脚手架入口
// ============================================================================
//
// 设计目标: 对标 SpringBoot 的 @SpringBootApplication,让用户以链式声明方式
// 注册模块,自动按依赖顺序执行 init(),应用退出时按逆序执行 cleanup()。
//
// 用法:
//   int main(int argc, char* argv[]) {
//       sc::Scaffold scaffold(argc, argv);
//       scaffold.use(MyLoggingModule{})
//              .use(MyNetworkModule{})
//              .use(MyStorageModule{});
//       return scaffold.run();
//   }
//
// 生命周期约定:
//   - Module 实例的生命周期由调用方管理(引用/指针),Scaffold 不拥有。
//   - Scaffold 析构时若已 run(),会自动按逆序调用已 init 成功模块的 cleanup()。
//   - 任一 Module::init() 失败时,Scaffold 会按逆序回滚已成功的模块并返回 -1。
//
// 与 sc::Application 的关系:
//   Scaffold 内部组合一个 Application 实例,委托其管理 QCoreApplication 与
//   startup/shutdown 回调。Scaffold 是 Application 之上的"声明式封装层"。
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
    // 模块按注册顺序 init(),按逆序 cleanup()。
    Scaffold& use(Module& module);
    Scaffold& use(Module* module);

    // 从模块注册表扫描并注册所有模块(对标 SpringBoot 自动配置) [v1.9.1 新增]
    // 模块实例由 Scaffold 管理生命周期,scan() 返回的模块会在析构时自动清理。
    // 与手动 use() 兼容:scan() 的模块先注册,手动 use() 的模块后注册。
    Scaffold& scan(ModuleRegistry& registry);

    // 启动应用。
    //   - 按注册顺序调用 Module::init()
    //   - 任一失败: 按逆序 cleanup() 已成功模块,返回 -1
    //   - 全部成功: 进入 QCoreApplication::exec() 事件循环
    //   - 事件循环退出: 自动按逆序 cleanup() 所有模块
    //   - [v1.9.2] 仅允许从 Uninitialized 状态调用,重复调用返回 -1
    int run();

    // 主动关闭(通常不需要手动调用,析构函数会处理)。
    // 幂等: 多次调用安全。
    void shutdown();

    // 获取底层 Application 实例(用于注册额外的 startup/shutdown 回调)。
    Application* application() { return m_app.get(); }

    /// @brief 获取当前运行状态 [v1.9.2 新增]
    State state() const { return m_state; }

private:
    std::unique_ptr<Application> m_app;
    std::vector<Module*> m_modules;
    std::vector<std::unique_ptr<Module>> m_ownedModules;  ///< v1.9.1: scan() 创建的模块,Scaffold 拥有生命周期
    std::vector<Module*> m_sortedModules;  // 拓扑排序后的模块列表(仅含 enabled),供 shutdown 按逆序 cleanup
    bool m_initialized = false;
    State m_state = State::Uninitialized;  ///< [v1.9.2] 运行状态跟踪
};

} // namespace sc

#endif // SOUL_CORE_SCAFFOLD_H
