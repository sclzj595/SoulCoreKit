#ifndef SOUL_CORE_APPLICATION_H
#define SOUL_CORE_APPLICATION_H

// ============================================================================
// application.h — SoulCoreKit 应用启动器 [v2.0.0 增强]
// ============================================================================
//
// 对标 SpringBoot 的 SpringApplication.run()。
// 提供一键启动: YAML 配置加载 → Profile 激活 → 模块扫描 → DI 初始化 → HTTP Server 启动。
//
// 用法:
//   int main(int argc, char* argv[]) {
//       return sc::Application::run(argc, argv);
//   }
//
// 或继承 Application 进行自定义:
//   class MyApp : public sc::Application {
//   protected:
//       void configure() override {
//           setConfigFile("application.yml");
//           setActiveProfile("dev");
//       }
//   };
//   int main(int argc, char* argv[]) {
//       return MyApp::run(argc, argv);
//   }

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QCoreApplication>

namespace sc {

class ModuleRegistry;
class Module;

// ============================================================================
// ApplicationState — 应用状态枚举
// ============================================================================
enum class ApplicationState {
    Created,     ///< 构造完成,尚未启动
    Starting,    ///< 正在启动(配置加载→模块初始化)
    Running,     ///< 事件循环运行中
    Stopping,    ///< 正在关闭
    Stopped      ///< 已关闭
};

// ============================================================================
// Application — 核心启动器
// ============================================================================
class Application {
public:
    using StartupCallback = std::function<void()>;
    using ShutdownCallback = std::function<void()>;

    // ========================================================================
    // 静态入口
    // ========================================================================

    /// @brief 一键启动(使用默认配置)
    /// @param argc 参数数量
    /// @param argv 参数数组
    /// @return 退出码
    static int run(int argc, char** argv);

    /// @brief 使用自定义 Application 子类启动
    template<typename AppType>
    static int run(int argc, char** argv) {
        static_assert(std::is_base_of<Application, AppType>::value,
                      "AppType must be derived from sc::Application");
        AppType app(argc, argv);
        return app.execute();
    }

    // ========================================================================
    // 构造与析构
    // ========================================================================

    Application(int argc = 0, char** argv = nullptr);
    virtual ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // ========================================================================
    // 配置方法(链式调用)
    // ========================================================================

    /// @brief 设置配置文件路径(默认 "application.yml")
    Application& setConfigFile(const std::string& path);

    /// @brief 设置激活的 Profile(默认从 APP_PROFILE 环境变量读取)
    Application& setActiveProfile(const std::string& profile);

    /// @brief 设置服务器端口(覆盖配置文件)
    Application& setServerPort(int port);

    /// @brief 设置服务器地址(覆盖配置文件)
    Application& setServerHost(const std::string& host);

    /// @brief 启用/禁用自动扫描模块(默认启用)
    Application& setAutoScanEnabled(bool enabled);

    /// @brief 注册 startup 回调
    Application& onStartup(StartupCallback callback);

    /// @brief 注册 shutdown 回调
    Application& onShutdown(ShutdownCallback callback);

    /// @brief 注册手动管理的模块(外部拥有生命周期) [v2.0.0 新增]
    Application& use(Module& module);

    /// @brief 注册手动管理的模块(指针形式) [v2.0.0 新增]
    Application& use(Module* module);

    /// @brief 从模块注册表扫描并注册所有模块 [v2.0.0 新增]
    Application& scan(ModuleRegistry& registry);

    // ========================================================================
    // 生命周期钩子(子类可重写)
    // ========================================================================

    /// @brief 配置阶段:在加载配置之前调用
    virtual void configure() {}

    /// @brief 模块注册阶段:在自动扫描之后、初始化之前调用
    virtual void registerModules() {}

    /// @brief 启动完成回调
    virtual void onStarted() {}

    /// @brief 关闭前回调
    virtual void onStopping() {}

    // ========================================================================
    // 状态查询
    // ========================================================================

    ApplicationState state() const { return m_state.load(); }

    /// @brief 请求关闭(优雅停机)
    void shutdown(int timeoutMs = 5000);

    /// @brief 获取底层 QCoreApplication
    QCoreApplication* qtApp() { return m_qtApp.get(); }

    /// @brief 执行完整启动流程 [v2.0.0 公开, 供 Scaffold 委托]
    int execute();

    int argc() const { return m_argc; }
    char** argv() const { return m_argv; }

protected:
    void setArgc(int argc) { m_argc = argc; }
    void setArgv(char** argv) { m_argv = argv; }

private:
    /// @brief 加载配置
    bool loadConfiguration();

    /// @brief 扫描并注册模块
    void scanAndRegisterModules();

    /// @brief 初始化模块(含拓扑排序 + 依赖解析 + 回滚) [v2.0.0 增强]
    bool initializeModules();

    /// @brief 启动模块(调用 onStart) [v2.0.0 新增]
    bool startModules();

    /// @brief 停止模块(调用 onStop) [v2.0.0 新增]
    void stopModules();

    /// @brief 清理模块(调用 cleanup) [v2.0.0 新增]
    void cleanupModules();

    /// @brief 启动服务
    void startServices();

    /// @brief 打印启动横幅
    void printBanner();

    int m_argc = 0;
    char** m_argv = nullptr;
    std::unique_ptr<QCoreApplication> m_qtApp;
    std::atomic<ApplicationState> m_state{ApplicationState::Created};

    // 配置
    std::string m_configFile = "application.yml";
    std::string m_activeProfile;
    int m_serverPort = -1;
    std::string m_serverHost;
    bool m_autoScanEnabled = true;

    // 回调
    std::vector<StartupCallback> m_startupCallbacks;
    std::vector<ShutdownCallback> m_shutdownCallbacks;

    // 模块管理
    std::vector<Module*> m_manualModules;       ///< use() 注册的外部模块(不拥有)
    std::vector<std::unique_ptr<Module>> m_ownedModules;  ///< scan() 创建的模块(拥有)
    std::vector<Module*> m_sortedModules;       ///< 拓扑排序后所有 enabled 模块(不拥有)
    std::vector<Module*> m_initializedModules;  ///< 已成功 init 的模块(用于回滚)
    std::vector<Module*> m_startedModules;      ///< 已成功 onStart 的模块(用于回滚) [v2.0.0]
};

} // namespace sc

#endif // SOUL_CORE_APPLICATION_H