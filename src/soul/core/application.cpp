#include "soul/core/application.h"
#include "soul/core/configuration.h"
#include "soul/core/module.h"
#include "soul/core/module_registry.h"
#include "soul/core/environment.h"
#include "soul/core/banner.h"
#include "soul/core/startup_logger.h"
#include "soul/logging/log_macros.h"
#include <QTimer>
#include <exception>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace sc {

// ============================================================================
// 静态入口
// ============================================================================

int Application::run(int argc, char** argv) {
    Application app(argc, argv);
    return app.execute();
}

// ============================================================================
// 构造与析构
// ============================================================================

Application::Application(int argc, char** argv) {
    m_argc = argc;
    m_argv = argv;
    m_qtApp = std::make_unique<QCoreApplication>(argc, argv);
}

Application::~Application() = default;

// ============================================================================
// 链式配置方法
// ============================================================================

Application& Application::setConfigFile(const std::string& path) {
    m_configFile = path;
    return *this;
}

Application& Application::setActiveProfile(const std::string& profile) {
    m_activeProfile = profile;
    return *this;
}

Application& Application::setServerPort(int port) {
    m_serverPort = port;
    return *this;
}

Application& Application::setServerHost(const std::string& host) {
    m_serverHost = host;
    return *this;
}

Application& Application::setAutoScanEnabled(bool enabled) {
    m_autoScanEnabled = enabled;
    return *this;
}

Application& Application::onStartup(StartupCallback callback) {
    m_startupCallbacks.push_back(std::move(callback));
    return *this;
}

Application& Application::onShutdown(ShutdownCallback callback) {
    m_shutdownCallbacks.push_back(std::move(callback));
    return *this;
}

Application& Application::use(Module& module) {
    m_manualModules.push_back(&module);
    return *this;
}

Application& Application::use(Module* module) {
    if (module) {
        m_manualModules.push_back(module);
    }
    return *this;
}

Application& Application::scan(ModuleRegistry& registry) {
    // 使用已注册模块名称集合防重入
    std::unordered_set<std::string> registeredNames;
    for (auto* m : m_manualModules) {
        registeredNames.insert(m->name());
    }

    auto factories = registry.factories();
    for (auto& [name, factory] : factories) {
        if (registeredNames.count(name)) {
            SC_INFO(std::string("Application: module '") + name + "' already registered, skipped");
            continue;
        }
        auto module = factory();
        if (module) {
            SC_INFO(std::string("Application: auto-discovered module '") + name + "'");
            m_manualModules.push_back(module.get());
            m_ownedModules.push_back(std::move(module));
        }
    }
    return *this;
}

// ============================================================================
// 生命周期 — execute()
// ============================================================================

int Application::execute() {
    // 防重入: 仅允许从 Created 状态执行
    if (m_state.load() != ApplicationState::Created) {
        std::cerr << "[ERROR] Application::execute() called from invalid state" << std::endl;
        return -1;
    }

    // a. 调用 configure() 钩子
    try {
        configure();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Application::configure() threw exception: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "[ERROR] Application::configure() threw unknown exception" << std::endl;
        return -1;
    }

    // b. 打印 Banner
    printBanner();

    // c. 加载配置
    if (!loadConfiguration()) {
        std::cerr << "[ERROR] Failed to load configuration" << std::endl;
        return -1;
    }

    // d. 设置状态为 Starting
    m_state = ApplicationState::Starting;

    // e. 调用 registerModules() 钩子
    try {
        registerModules();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Application::registerModules() threw exception: " << e.what() << std::endl;
        return -1;
    } catch (...) {
        std::cerr << "[ERROR] Application::registerModules() threw unknown exception" << std::endl;
        return -1;
    }

    // f. 扫描并注册模块
    if (m_autoScanEnabled) {
        scanAndRegisterModules();
    }

    // g. 初始化模块(含拓扑排序 + 依赖解析 + 回滚)
    if (!initializeModules()) {
        std::cerr << "[ERROR] Failed to initialize modules" << std::endl;
        return -1;
    }

    // h. 启动模块(调用 onStart)
    if (!startModules()) {
        std::cerr << "[ERROR] Failed to start modules" << std::endl;
        return -1;
    }

    // i. 启动服务(打印端口等诊断信息)
    startServices();

    // j. 调用 onStarted() 钩子 + 执行 startup callbacks
    try {
        onStarted();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Application::onStarted() threw exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Application::onStarted() threw unknown exception" << std::endl;
    }
    for (auto& cb : m_startupCallbacks) {
        try {
            cb();
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] startup callback threw exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] startup callback threw unknown exception" << std::endl;
        }
    }

    // k. 设置状态为 Running
    m_state = ApplicationState::Running;

    // l. 进入事件循环
    int exitCode = m_qtApp->exec();

    // m. 事件循环退出后: 优雅停机
    m_state = ApplicationState::Stopping;

    try {
        onStopping();
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Application::onStopping() threw exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "[ERROR] Application::onStopping() threw unknown exception" << std::endl;
    }

    // 逆序停止模块
    stopModules();

    // 逆序清理模块
    cleanupModules();

    for (auto& cb : m_shutdownCallbacks) {
        try {
            cb();
        } catch (const std::exception& e) {
            std::cerr << "[ERROR] shutdown callback threw exception: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[ERROR] shutdown callback threw unknown exception" << std::endl;
        }
    }

    m_state = ApplicationState::Stopped;

    return exitCode;
}

// ============================================================================
// shutdown()
// ============================================================================

void Application::shutdown(int timeoutMs) {
    // 超时强制退出: 若 timeoutMs 毫秒后事件循环仍未退出,强制 quit
    QTimer::singleShot(timeoutMs, m_qtApp.get(), &QCoreApplication::quit);
    // 触发优雅停机
    m_qtApp->quit();
}

// ============================================================================
// 配置加载
// ============================================================================

bool Application::loadConfiguration() {
    auto& cfg = Configuration::instance();

    // 加载 application.yml
    if (!cfg.loadFromFile(m_configFile)) {
        std::cout << "[WARN] Could not load " << m_configFile << ", using defaults" << std::endl;
    }

    // 激活 Profile
    std::string profile = m_activeProfile;
    if (profile.empty()) {
        profile = Environment::get("APP_PROFILE", "");
    }
    if (!profile.empty()) {
        cfg.setActiveProfile(profile);
        std::string profileFile = "application-" + profile + ".yml";
        cfg.loadFromFile(profileFile);
    }

    // 解析命令行参数
    cfg.parseCommandLine(m_argc, m_argv);

    return true;
}

// ============================================================================
// 模块扫描
// ============================================================================

void Application::scanAndRegisterModules() {
    scan(ModuleRegistry::instance());
}

// ============================================================================
// 拓扑排序(含依赖解析 + 优先级排序) [v2.0.0 新增]
// ============================================================================

namespace {

std::vector<Module*> topoSortWithPriority(const std::vector<Module*>& modules) {
    std::unordered_map<std::string, std::size_t> nameToIdx;
    for (std::size_t i = 0; i < modules.size(); ++i) {
        nameToIdx[modules[i]->name()] = i;
    }

    std::vector<std::vector<std::size_t>> graph(modules.size());
    std::vector<std::size_t> indegree(modules.size(), 0);

    for (std::size_t i = 0; i < modules.size(); ++i) {
        for (const auto& dep : modules[i]->dependsOn()) {
            auto it = nameToIdx.find(dep);
            if (it == nameToIdx.end()) {
                SC_ERROR(std::string("Application: module '") + modules[i]->name() +
                         "' depends on unknown module '" + dep + "'");
                return {};
            }
            graph[it->second].push_back(i);
            ++indegree[i];
        }
    }

    std::vector<Module*> result;
    result.reserve(modules.size());
    std::vector<bool> visited(modules.size(), false);

    while (result.size() < modules.size()) {
        int bestIdx = -1;
        int bestPriority = 0;
        for (std::size_t i = 0; i < modules.size(); ++i) {
            if (!visited[i] && indegree[i] == 0) {
                int p = modules[i]->priority();
                if (bestIdx == -1 || p > bestPriority) {
                    bestIdx = static_cast<int>(i);
                    bestPriority = p;
                }
            }
        }
        if (bestIdx == -1) {
            SC_ERROR("Application: circular dependency detected among modules");
            return {};
        }
        visited[bestIdx] = true;
        result.push_back(modules[bestIdx]);
        for (std::size_t next : graph[bestIdx]) {
            if (indegree[next] > 0) --indegree[next];
        }
    }
    return result;
}

} // namespace

// ============================================================================
// 模块初始化(含拓扑排序 + 依赖解析 + 回滚) [v2.0.0 增强]
// ============================================================================

bool Application::initializeModules() {
    // 1. 收集所有模块(手动 + 自动扫描)
    std::vector<Module*> allModules = m_manualModules;

    // 2. 过滤 isEnabled() == false 的模块
    std::vector<Module*> enabledModules;
    for (auto* m : allModules) {
        if (m->isEnabled()) {
            enabledModules.push_back(m);
        } else {
            SC_INFO(std::string("Application: module '") + m->name() + "' disabled, skipped");
        }
    }

    if (enabledModules.empty()) {
        return true;
    }

    // 3. 拓扑排序 + 优先级排序
    auto sorted = topoSortWithPriority(enabledModules);
    if (sorted.empty()) {
        SC_ERROR("Application: module sorting failed (circular dependency or missing dep)");
        return false;
    }

    m_sortedModules = sorted;

    // 4. 按排序顺序执行 init()
    for (auto* module : sorted) {
        SC_INFO(std::string("Application: initializing module '") + module->name() + "'");
        auto result = module->init();
        if (result.isErr()) {
            const QString msg = result.unwrapErr().message();
            SC_ERROR(QString("Application: module '%1' init failed: %2")
                         .arg(QString::fromStdString(module->name()))
                         .arg(msg)
                         .toStdString());
            // 逆序回滚已成功 init 的模块
            for (auto it = m_initializedModules.rbegin(); it != m_initializedModules.rend(); ++it) {
                try {
                    (*it)->cleanup();
                    SC_INFO(std::string("Application: rolled back module '") + (*it)->name() + "'");
                } catch (const std::exception& e) {
                    SC_ERROR(std::string("Application: rollback of '") + (*it)->name() +
                             "' failed: " + e.what());
                }
            }
            m_initializedModules.clear();
            return false;
        }
        m_initializedModules.push_back(module);
        SC_INFO(std::string("Application: module '") + module->name() + "' initialized");
    }

    return true;
}

// ============================================================================
// 模块启动(调用 onStart) [v2.0.0 新增]
// ============================================================================

bool Application::startModules() {
    for (auto* module : m_initializedModules) {
        SC_INFO(std::string("Application: starting module '") + module->name() + "'");
        auto result = module->onStart();
        if (result.isErr()) {
            const QString msg = result.unwrapErr().message();
            SC_ERROR(QString("Application: module '%1' start failed: %2")
                         .arg(QString::fromStdString(module->name()))
                         .arg(msg)
                         .toStdString());
            // 逆序 stop 已 start 的模块
            for (auto it = m_startedModules.rbegin(); it != m_startedModules.rend(); ++it) {
                try {
                    (*it)->onStop();
                } catch (const std::exception& e) {
                    SC_ERROR(std::string("Application: stop of '") + (*it)->name() +
                             "' failed: " + e.what());
                }
            }
            // 逆序 cleanup 已 init 的模块
            for (auto it = m_initializedModules.rbegin(); it != m_initializedModules.rend(); ++it) {
                try {
                    (*it)->cleanup();
                } catch (const std::exception& e) {
                    SC_ERROR(std::string("Application: cleanup of '") + (*it)->name() +
                             "' failed: " + e.what());
                }
            }
            m_startedModules.clear();
            m_initializedModules.clear();
            return false;
        }
        m_startedModules.push_back(module);
        SC_INFO(std::string("Application: module '") + module->name() + "' started");
    }
    return true;
}

// ============================================================================
// 模块停止(调用 onStop) [v2.0.0 新增]
// ============================================================================

void Application::stopModules() {
    for (auto it = m_startedModules.rbegin(); it != m_startedModules.rend(); ++it) {
        try {
            (*it)->onStop();
            SC_INFO(std::string("Application: module '") + (*it)->name() + "' stopped");
        } catch (const std::exception& e) {
            SC_ERROR(std::string("Application: module '") + (*it)->name() +
                     "' onStop failed: " + e.what());
        }
    }
    m_startedModules.clear();
}

// ============================================================================
// 模块清理(调用 cleanup) [v2.0.0 新增]
// ============================================================================

void Application::cleanupModules() {
    for (auto it = m_initializedModules.rbegin(); it != m_initializedModules.rend(); ++it) {
        try {
            (*it)->cleanup();
            SC_INFO(std::string("Application: module '") + (*it)->name() + "' cleaned up");
        } catch (const std::exception& e) {
            SC_ERROR(std::string("Application: module '") + (*it)->name() +
                     "' cleanup failed: " + e.what());
        }
    }
    m_initializedModules.clear();
    m_ownedModules.clear();
    m_manualModules.clear();
    m_sortedModules.clear();
}

// ============================================================================
// 服务启动
// ============================================================================

void Application::startServices() {
    StartupLogger logger;
    logger.start();

    auto& cfg = Configuration::instance();
    int port = m_serverPort > 0 ? m_serverPort : cfg.serverPort();
    std::string host = m_serverHost.empty() ? cfg.serverHost() : m_serverHost;
    std::string profile = cfg.activeProfile();

    logger.logPort(port);
    logger.logHost(host);
    logger.logConfigFile(m_configFile);
    if (!profile.empty()) {
        logger.logProfile(profile);
    }

    for (auto* module : m_initializedModules) {
        logger.logModule(module->name(), true);
    }

    logger.printSummary();
}

// ============================================================================
// Banner
// ============================================================================

void Application::printBanner() {
    Banner::print("2.0.0");
}

} // namespace sc