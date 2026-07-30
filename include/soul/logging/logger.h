#ifndef SOUL_LOGGING_LOGGER_H
#define SOUL_LOGGING_LOGGER_H

#include "soul/logging/log_level.h"
#include "soul/logging/log_record.h"
#include "soul/logging/i_sink.h"
#include "soul/logging/composite_sink.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <QString>
#include <mutex>

namespace sc {

class Logger {
public:
    static Logger& instance();

    void setLogLevel(LogLevel level);
    LogLevel logLevel() const;

    /// @brief 设置模块级日志级别 [v1.9.2 新增]
    /// @param module 模块名称(如 "orm", "network", "auth")
    /// @param level  最低输出级别
    void setModuleLogLevel(const std::string& module, LogLevel level);

    /// @brief 获取模块级日志级别 [v1.9.2 新增]
    /// @param module 模块名称
    /// @return 模块日志级别,未设置时返回全局级别
    LogLevel moduleLogLevel(const std::string& module) const;

    /// @brief 移除模块级日志级别(恢复全局级别) [v1.9.2 新增]
    void removeModuleLogLevel(const std::string& module);

    void addSink(std::shared_ptr<ISink> sink);
    void removeSink(ISink* sink);

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

    void flush();
    void init();
    void shutdown();

private:
    Logger();

    LogLevel m_level = LogLevel::Debug;
    std::unordered_map<std::string, LogLevel> m_moduleLevels;  ///< [v1.9.2] 模块级日志级别
    std::shared_ptr<CompositeSink> m_sink;
    mutable std::mutex m_mutex;
};

}

#endif
