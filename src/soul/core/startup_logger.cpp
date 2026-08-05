#include "soul/core/startup_logger.h"
#include <sstream>

namespace sc {

StartupLogger::StartupLogger() {
    m_startTime = std::chrono::steady_clock::now();
}

void StartupLogger::start() {
    m_startTime = std::chrono::steady_clock::now();
}

void StartupLogger::logPort(int port) {
    m_port = port;
}

void StartupLogger::logHost(const std::string& host) {
    m_host = host;
}

void StartupLogger::logModule(const std::string& moduleName, bool enabled) {
    m_modules.push_back({moduleName, enabled});
}

void StartupLogger::logProfile(const std::string& profile) {
    m_profile = profile;
}

void StartupLogger::logConfigFile(const std::string& path) {
    m_configFile = path;
}

int64_t StartupLogger::elapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - m_startTime).count();
}

void StartupLogger::printSummary() {
    auto elapsed = elapsedMs();

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "  SoulCoreKit Startup Summary" << std::endl;
    std::cout << "========================================" << std::endl;

    if (!m_configFile.empty()) {
        std::cout << "  Config file : " << m_configFile << std::endl;
    }
    if (!m_profile.empty()) {
        std::cout << "  Profile     : " << m_profile << std::endl;
    }
    if (m_port > 0) {
        std::cout << "  Server      : " << m_host << ":" << m_port << std::endl;
    }

    // 模块列表
    std::cout << "  Modules     :" << std::endl;
    int enabledCount = 0;
    for (const auto& m : m_modules) {
        std::cout << "    " << (m.enabled ? "[+] " : "[-] ") << m.name << std::endl;
        if (m.enabled) ++enabledCount;
    }
    std::cout << "  Total modules: " << m_modules.size()
              << " (enabled: " << enabledCount << ")" << std::endl;

    std::cout << "  Startup time: " << elapsed << " ms" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
}

} // namespace sc