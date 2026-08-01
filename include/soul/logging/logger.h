#ifndef SOUL_LOGGING_LOGGER_H
#define SOUL_LOGGING_LOGGER_H

#include "soul/logging/log_level.h"
#include "soul/logging/log_record.h"
#include "soul/logging/i_sink.h"
#include "soul/logging/composite_sink.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <QString>
#include <mutex>

namespace sc {

// ============================================================================
// 日志格式化工具函数
// ============================================================================

/// @brief 将 sc::LogLevel 映射为 spdlog::level::level_enum
inline spdlog::level::level_enum toSpdlogLevel(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return spdlog::level::trace;
    case LogLevel::Debug: return spdlog::level::debug;
    case LogLevel::Info:  return spdlog::level::info;
    case LogLevel::Warn:  return spdlog::level::warn;
    case LogLevel::Error: return spdlog::level::err;
    case LogLevel::Fatal: return spdlog::level::critical;
    default:              return spdlog::level::info;
    }
}

/// @brief 将 spdlog::level::level_enum 映射回 sc::LogLevel
inline LogLevel fromSpdlogLevel(spdlog::level::level_enum level) {
    switch (level) {
    case spdlog::level::trace:    return LogLevel::Trace;
    case spdlog::level::debug:    return LogLevel::Debug;
    case spdlog::level::info:     return LogLevel::Info;
    case spdlog::level::warn:     return LogLevel::Warn;
    case spdlog::level::err:      return LogLevel::Error;
    case spdlog::level::critical: return LogLevel::Fatal;
    default:                      return LogLevel::Info;
    }
}

class Logger {
public:
    static Logger& instance();

    // ========================================================================
    // 日志级别控制
    // ========================================================================

    void setLogLevel(LogLevel level);
    LogLevel logLevel() const;

    /// @brief 设置模块级日志级别 [v1.9.2 新增]
    void setModuleLogLevel(const std::string& module, LogLevel level);

    /// @brief 获取模块级日志级别 [v1.9.2 新增]
    LogLevel moduleLogLevel(const std::string& module) const;

    /// @brief 移除模块级日志级别 [v1.9.2 新增]
    void removeModuleLogLevel(const std::string& module);

    // ========================================================================
    // Sink 管理 (兼容旧版 ISink 体系)
    // ========================================================================

    void addSink(std::shared_ptr<ISink> sink);
    void removeSink(ISink* sink);

    // ========================================================================
    // spdlog 原生 Sink 管理 [v1.9.3 新增]
    // ========================================================================

    /// @brief 添加控制台输出 (带颜色)
    void addConsoleSink();

    /// @brief 添加滚动文件输出
    /// @param filePath 日志文件路径
    /// @param maxFileSize 单个文件最大大小(字节)
    /// @param maxFiles 最大保留文件数
    void addRotatingFileSink(const std::string& filePath,
                             size_t maxFileSize = 10 * 1024 * 1024,
                             size_t maxFiles = 5);

    /// @brief 添加按天滚动文件输出
    /// @param filePath 日志文件路径
    /// @param hour 每天滚动时间(小时, 0-23)
    /// @param minute 每天滚动时间(分钟, 0-59)
    void addDailyFileSink(const std::string& filePath,
                          int hour = 0, int minute = 0);

    // ========================================================================
    // 日志输出 API (兼容旧版)
    // ========================================================================

    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const QString& message);
    void log(LogLevel level, const std::string& message,
             const std::string& module, const std::string& operation = "");
    void log(LogLevel level, const QString& message,
             const QString& module, const QString& operation = "");

    void trace(const char* message) { log(LogLevel::Trace, std::string(message)); }
    void trace(const std::string& message) { log(LogLevel::Trace, message); }
    void trace(const QString& message) { log(LogLevel::Trace, message); }

    void debug(const char* message) { log(LogLevel::Debug, std::string(message)); }
    void debug(const std::string& message) { log(LogLevel::Debug, message); }
    void debug(const QString& message) { log(LogLevel::Debug, message); }

    void info(const char* message) { log(LogLevel::Info, std::string(message)); }
    void info(const std::string& message) { log(LogLevel::Info, message); }
    void info(const QString& message) { log(LogLevel::Info, message); }

    void warn(const char* message) { log(LogLevel::Warn, std::string(message)); }
    void warn(const std::string& message) { log(LogLevel::Warn, message); }
    void warn(const QString& message) { log(LogLevel::Warn, message); }

    void error(const char* message) { log(LogLevel::Error, std::string(message)); }
    void error(const std::string& message) { log(LogLevel::Error, message); }
    void error(const QString& message) { log(LogLevel::Error, message); }

    void fatal(const char* message) { log(LogLevel::Fatal, std::string(message)); }
    void fatal(const std::string& message) { log(LogLevel::Fatal, message); }
    void fatal(const QString& message) { log(LogLevel::Fatal, message); }

    void trace(const char* message, const char* module, const char* operation = "") {
        log(LogLevel::Trace, std::string(message), std::string(module), std::string(operation));
    }
    void trace(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Trace, message, module, operation); }
    void trace(const QString& message, const QString& module, const QString& operation = "") { log(LogLevel::Trace, message, module, operation); }

    void debug(const char* message, const char* module, const char* operation = "") {
        log(LogLevel::Debug, std::string(message), std::string(module), std::string(operation));
    }
    void debug(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Debug, message, module, operation); }
    void debug(const QString& message, const QString& module, const QString& operation = "") { log(LogLevel::Debug, message, module, operation); }

    void info(const char* message, const char* module, const char* operation = "") {
        log(LogLevel::Info, std::string(message), std::string(module), std::string(operation));
    }
    void info(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Info, message, module, operation); }
    void info(const QString& message, const QString& module, const QString& operation = "") { log(LogLevel::Info, message, module, operation); }

    void warn(const char* message, const char* module, const char* operation = "") {
        log(LogLevel::Warn, std::string(message), std::string(module), std::string(operation));
    }
    void warn(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Warn, message, module, operation); }
    void warn(const QString& message, const QString& module, const QString& operation = "") { log(LogLevel::Warn, message, module, operation); }

    void error(const char* message, const char* module, const char* operation = "") {
        log(LogLevel::Error, std::string(message), std::string(module), std::string(operation));
    }
    void error(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Error, message, module, operation); }
    void error(const QString& message, const QString& module, const QString& operation = "") { log(LogLevel::Error, message, module, operation); }

    void fatal(const char* message, const char* module, const char* operation = "") {
        log(LogLevel::Fatal, std::string(message), std::string(module), std::string(operation));
    }
    void fatal(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Fatal, message, module, operation); }
    void fatal(const QString& message, const QString& module, const QString& operation = "") { log(LogLevel::Fatal, message, module, operation); }

    // ========================================================================
    // fmt 风格格式化日志 [v1.9.3 新增]
    // ========================================================================
    // 用法: Logger::instance().logFmt(LogLevel::Info, "User {} logged in", userId);
    //       Logger::instance().infoFmt("User {} logged in from {}", name, ip);

    template<typename... Args>
    void logFmt(LogLevel level, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        checkAndLog(level, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void traceFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        checkAndLog(LogLevel::Trace, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void debugFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        checkAndLog(LogLevel::Debug, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void infoFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        checkAndLog(LogLevel::Info, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void warnFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        checkAndLog(LogLevel::Warn, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void errorFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        checkAndLog(LogLevel::Error, fmt, std::forward<Args>(args)...);
    }

    template<typename... Args>
    void fatalFmt(spdlog::format_string_t<Args...> fmt, Args&&... args) {
        checkAndLog(LogLevel::Fatal, fmt, std::forward<Args>(args)...);
    }

    // ========================================================================
    // 生命周期管理
    // ========================================================================

    void flush();
    void init();
    void shutdown();

    /// @brief 获取底层 spdlog::logger (供高级用法)
    std::shared_ptr<spdlog::logger> spdlogLogger() { return m_spdlogLogger; }

private:
    Logger();

    template<typename... Args>
    void checkAndLog(LogLevel level, spdlog::format_string_t<Args...> fmt, Args&&... args) {
        // 全局级别检查(atomic 读,防止与 setLogLevel 并发数据竞争)
        if (level < m_level.load(std::memory_order_acquire)) return;

        // 持锁防止与 add*Sink 并发修改 sinks 向量
        std::lock_guard<std::mutex> lock(m_mutex);
        m_spdlogLogger->log(toSpdlogLevel(level), fmt, std::forward<Args>(args)...);
    }

    std::atomic<LogLevel> m_level{LogLevel::Debug};
    std::unordered_map<std::string, LogLevel> m_moduleLevels;
    std::shared_ptr<CompositeSink> m_legacySink;   ///< 兼容旧版 ISink
    std::shared_ptr<spdlog::logger> m_spdlogLogger;
    mutable std::mutex m_mutex;
};

}

#endif