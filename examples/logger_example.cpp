#include <memory>
#include "soul/logging/log_macros.h"
#include "soul/logging/file_sink.h"

int main() {
    sc::Logger::instance().setLogLevel(sc::LogLevel::Trace);

    // 旧版 ISink 兼容
    auto fileSink = std::make_shared<sc::FileSink>("logs/soul_core.log");
    sc::Logger::instance().addSink(fileSink);

    // spdlog 原生 Sink (v1.9.3 新增)
    sc::Logger::instance().addRotatingFileSink("logs/app.log", 10 * 1024 * 1024, 5);
    sc::Logger::instance().addDailyFileSink("logs/daily.log", 0, 0);

    // 兼容旧版宏
    SC_TRACE("This is a trace message");
    SC_DEBUG("This is a debug message");
    SC_INFO("This is an info message");
    SC_WARN("This is a warning message");
    SC_ERROR("This is an error message");

    SC_INFO_S("Network", "connect", "Connected to server");
    SC_WARN_S("Storage", "cache", "Cache miss for key user_123");
    SC_ERROR_S("Database", "query", "Failed to execute query");

    // 模块级日志
    SC_LOGC_INFO(orm, "Migration completed");
    SC_LOGC_WARN(network, "Connection timeout");

    // fmt 风格格式化 (v1.9.3 新增)
    SC_INFO_FMT("Server started on port {}", 8080);
    SC_DEBUG_FMT("User {} logged in from {}", "admin", "192.168.1.1");
    SC_WARN_FMT("Connection timeout after {}ms", 5000);
    SC_ERROR_FMT("Failed to open file: {}", "config.json");

    // spdlog 原生宏 (v1.9.3 新增)
    SC_LOG_INFO("spdlog native: value = {:.2f}", 3.14159);
    SC_LOG_DEBUG("spdlog native: hex = {:#x}", 255);

    return 0;
}