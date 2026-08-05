#ifndef SOUL_CORE_STARTUP_LOGGER_H
#define SOUL_CORE_STARTUP_LOGGER_H

// ============================================================================
// startup_logger.h — 启动诊断信息 [v2.0.0]
// ============================================================================
//
// 对标 SpringBoot 的 StartupInfoLogger。
// 启动时打印端口、模块列表、耗时等诊断信息。
//
// 用法:
//   StartupLogger logger;
//   logger.logPort(8080);
//   logger.logModule("LoggingModule", true);
//   logger.logModule("NetworkModule", false);
//   logger.logElapsed(1234);  // 启动耗时 1234ms

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

namespace sc {

class StartupLogger {
public:
    StartupLogger();

    /// @brief 记录启动开始时间
    void start();

    /// @brief 记录端口信息
    void logPort(int port);

    /// @brief 记录主机地址 [v2.0.0]
    void logHost(const std::string& host);

    /// @brief 记录模块状态
    void logModule(const std::string& moduleName, bool enabled);

    /// @brief 记录 Profile
    void logProfile(const std::string& profile);

    /// @brief 记录配置文件
    void logConfigFile(const std::string& path);

    /// @brief 打印启动摘要
    void printSummary();

    /// @brief 获取启动耗时(毫秒)
    int64_t elapsedMs() const;

private:
    std::chrono::steady_clock::time_point m_startTime;
    int m_port = -1;
    std::string m_host;
    std::string m_profile;
    std::string m_configFile;
    struct ModuleInfo {
        std::string name;
        bool enabled;
    };
    std::vector<ModuleInfo> m_modules;
};

} // namespace sc

#endif // SOUL_CORE_STARTUP_LOGGER_H