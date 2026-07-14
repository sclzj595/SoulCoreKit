#ifndef SOUL_LOGGING_LOGGER_H
#define SOUL_LOGGING_LOGGER_H

#include "soul/logging/log_level.h"
#include "soul/logging/log_record.h"
#include "soul/logging/i_sink.h"
#include "soul/logging/composite_sink.h"
#include "soul/core/singleton.h"
#include <memory>
#include <string>
#include <mutex>

namespace sc {

/**
 * @class Logger
 * @brief 日志门面类，提供分级日志记录功能
 *
 * Logger 是日志系统的入口类，采用单例模式。提供 Trace/Debug/Info/Warn/Error/Fatal
 * 六个级别的日志记录方法，支持添加多个输出目标（Sink）。
 *
 * 线程安全：所有方法均通过互斥锁保护，支持多线程并发调用。
 *
 * 使用方式：
 * @code
 * Logger::instance().info("Application started");
 * Logger::instance().info("User logged in", "Auth", "login");
 * Logger::instance().error("Connection failed", "Network");
 * @endcode
 *
 * @see LogLevel, ISink, Singleton
 */
class Logger : public Singleton<Logger> {
    friend class Singleton<Logger>;

public:
    /**
     * @brief 设置日志级别
     * @param level 日志级别，低于此级别的日志将被过滤
     */
    void setLogLevel(LogLevel level);

    /**
     * @brief 获取当前日志级别
     * @return 当前日志级别
     */
    LogLevel logLevel() const;

    /**
     * @brief 添加日志输出目标
     * @param sink 日志输出目标
     */
    void addSink(std::shared_ptr<ISink> sink);

    /**
     * @brief 移除日志输出目标
     * @param sink 日志输出目标指针
     */
    void removeSink(ISink* sink);

    /**
     * @brief 记录日志（通用方法）
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const std::string& message);

    /**
     * @brief 记录日志（带模块和操作）
     * @param level 日志级别
     * @param message 日志消息
     * @param module 模块名称
     * @param operation 操作名称
     */
    void log(LogLevel level, const std::string& message,
             const std::string& module, const std::string& operation = "");

    /**
     * @brief 记录 Trace 级别日志
     * @param message 日志消息
     */
    void trace(const std::string& message) { log(LogLevel::Trace, message); }

    /**
     * @brief 记录 Debug 级别日志
     * @param message 日志消息
     */
    void debug(const std::string& message) { log(LogLevel::Debug, message); }

    /**
     * @brief 记录 Info 级别日志
     * @param message 日志消息
     */
    void info(const std::string& message) { log(LogLevel::Info, message); }

    /**
     * @brief 记录 Warn 级别日志
     * @param message 日志消息
     */
    void warn(const std::string& message) { log(LogLevel::Warn, message); }

    /**
     * @brief 记录 Error 级别日志
     * @param message 日志消息
     */
    void error(const std::string& message) { log(LogLevel::Error, message); }

    /**
     * @brief 记录 Fatal 级别日志
     * @param message 日志消息
     */
    void fatal(const std::string& message) { log(LogLevel::Fatal, message); }

    /**
     * @brief 记录 Trace 级别日志（带模块和操作）
     * @param message 日志消息
     * @param module 模块名称
     * @param operation 操作名称
     */
    void trace(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Trace, message, module, operation); }

    /**
     * @brief 记录 Debug 级别日志（带模块和操作）
     * @param message 日志消息
     * @param module 模块名称
     * @param operation 操作名称
     */
    void debug(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Debug, message, module, operation); }

    /**
     * @brief 记录 Info 级别日志（带模块和操作）
     * @param message 日志消息
     * @param module 模块名称
     * @param operation 操作名称
     */
    void info(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Info, message, module, operation); }

    /**
     * @brief 记录 Warn 级别日志（带模块和操作）
     * @param message 日志消息
     * @param module 模块名称
     * @param operation 操作名称
     */
    void warn(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Warn, message, module, operation); }

    /**
     * @brief 记录 Error 级别日志（带模块和操作）
     * @param message 日志消息
     * @param module 模块名称
     * @param operation 操作名称
     */
    void error(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Error, message, module, operation); }

    /**
     * @brief 记录 Fatal 级别日志（带模块和操作）
     * @param message 日志消息
     * @param module 模块名称
     * @param operation 操作名称
     */
    void fatal(const std::string& message, const std::string& module, const std::string& operation = "") { log(LogLevel::Fatal, message, module, operation); }

    /**
     * @brief 刷新所有输出目标的缓冲区
     */
    void flush();

    /**
     * @brief 初始化日志系统
     */
    void init();

    /**
     * @brief 关闭日志系统
     */
    void shutdown();

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Logger();

    LogLevel m_level = LogLevel::Debug;
    std::shared_ptr<CompositeSink> m_sink;
    mutable std::mutex m_mutex;
};

}

#endif
