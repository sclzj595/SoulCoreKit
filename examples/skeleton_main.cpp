// ============================================================================
// skeleton_main.cpp — SoulCoreKit 最小骨架示例
// ============================================================================
// 演示脚手架"5 分钟搭起一个进程"的能力:
//   1. 一行 #include "soul/soul.h" 接入核心基础设施
//   2. 自定义 Module 继承 sc::Module
//   3. sc::Scaffold 链式声明式注册模块,自动按依赖顺序装配
//   4. 应用退出时自动按逆序 cleanup()
//
// 与 SpringBoot 的对应关系:
//   @SpringBootApplication  -> sc::Scaffold
//   @Component              -> sc::Module 子类
//   SpringApplication.run() -> scaffold.run()
//
// 构建: 已在 examples/CMakeLists.txt 中注册为 skeleton_example 目标
// 运行: 启动后 3 秒自动退出,日志输出模块初始化与清理顺序

#include "soul/soul.h"

#include <QTimer>
#include <iostream>

// ============================================================================
// 示例模块 1: Hello World 演示
// ============================================================================
class HelloWorldModule : public sc::Module {
public:
    HelloWorldModule() : sc::Module("HelloWorld") {}

    sc::Result<void> init() override {
        SC_INFO("HelloWorld module initialized");
        std::cout << "[HelloWorld] module initialized" << std::endl;
        return {};
    }

    void cleanup() override {
        SC_INFO("HelloWorld module cleaned up");
        std::cout << "[HelloWorld] module cleaned up" << std::endl;
    }
};

// ============================================================================
// 示例模块 2: 定时器模块,3 秒后退出应用
// ============================================================================
class TimerModule : public sc::Module {
public:
    TimerModule() : sc::Module("Timer") {}

    sc::Result<void> init() override {
        SC_INFO("Timer module initialized, will quit after 3 seconds");
        QTimer::singleShot(3000, []() {
            SC_INFO("Timer expired, quitting application");
            if (auto* app = QCoreApplication::instance()) {
                app->quit();
            }
        });
        return {};
    }

    void cleanup() override {
        SC_INFO("Timer module cleaned up");
    }
};

// ============================================================================
// 主入口:声明式脚手架装配
// ============================================================================
int main(int argc, char* argv[]) {
    // 1. 创建模块实例(生命周期由 main 管理)
    HelloWorldModule helloModule;
    TimerModule      timerModule;

    // 2. 声明式注册到 Scaffold
    sc::Scaffold scaffold(argc, argv);
    scaffold.use(helloModule)
           .use(timerModule);

    // 3. 启动应用(自动按顺序 init,退出时按逆序 cleanup)
    std::cout << "=== SoulCoreKit Skeleton Starting ===" << std::endl;
    const int exitCode = scaffold.run();
    std::cout << "=== SoulCoreKit Skeleton Exited with code: " << exitCode << " ===" << std::endl;
    return exitCode;
}
