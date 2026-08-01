#include <memory>
#include <mutex>
#include <string>
#include "soul/logging/logger.h"
#include "soul/logging/console_sink.h"
#include "soul/core/time.h"

namespace sc {

Logger::Logger()
    : m_legacySink(std::make_shared<CompositeSink>())
{
    // 创建 spdlog logger,默认带彩色控制台输出
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    consoleSink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");

    m_spdlogLogger = std::make_shared<spdlog::logger>("soulcore", consoleSink);
    m_spdlogLogger->set_level(spdlog::level::debug);
    m_spdlogLogger->flush_on(spdlog::level::err);

    // 注册为默认 logger,使 spdlog::info() 等全局函数可用
    spdlog::set_default_logger(m_spdlogLogger);
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

// ============================================================================
// 日志级别控制
// ============================================================================

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level.store(level, std::memory_order_release);
    if (m_spdlogLogger) {
        m_spdlogLogger->set_level(toSpdlogLevel(level));
    }
}

LogLevel Logger::logLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level.load(std::memory_order_acquire);
}

void Logger::setModuleLogLevel(const std::string& module, LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_moduleLevels[module] = level;
}

LogLevel Logger::moduleLogLevel(const std::string& module) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_moduleLevels.find(module);
    if (it != m_moduleLevels.end()) {
        return it->second;
    }
    return m_level.load(std::memory_order_acquire);
}

void Logger::removeModuleLogLevel(const std::string& module) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_moduleLevels.erase(module);
}

// ============================================================================
// Sink 管理
// ============================================================================

void Logger::addSink(std::shared_ptr<ISink> sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_legacySink->addSink(sink);
}

void Logger::removeSink(ISink* sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_legacySink->removeSink(sink);
}

void Logger::addConsoleSink() {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%t] %v");
    m_spdlogLogger->sinks().push_back(sink);
}

void Logger::addRotatingFileSink(const std::string& filePath,
                                  size_t maxFileSize, size_t maxFiles) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        filePath, maxFileSize, maxFiles);
    sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    m_spdlogLogger->sinks().push_back(sink);
}

void Logger::addDailyFileSink(const std::string& filePath,
                               int hour, int minute) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto sink = std::make_shared<spdlog::sinks::daily_file_sink_mt>(
        filePath, hour, minute);
    sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    m_spdlogLogger->sinks().push_back(sink);
}

// ============================================================================
// 日志输出
// ============================================================================

void Logger::log(LogLevel level, const std::string& message) {
    log(level, message, "", "");
}

void Logger::log(LogLevel level, const QString& message) {
    log(level, message.toStdString());
}

void Logger::log(LogLevel level, const std::string& message,
                 const std::string& module, const std::string& operation) {
    // 持锁读取级别配置和 logger/sink 指针,防止与 setLogLevel/addSink 等写入并发
    std::lock_guard<std::mutex> lock(m_mutex);

    // 模块级日志过滤
    if (!module.empty()) {
        auto it = m_moduleLevels.find(module);
        if (it != m_moduleLevels.end()) {
            if (level < it->second) return;
        } else if (level < m_level.load(std::memory_order_acquire)) {
            return;
        }
    } else if (level < m_level.load(std::memory_order_acquire)) {
        return;
    }

    // 通过 spdlog 输出
    if (!module.empty() && !operation.empty()) {
        m_spdlogLogger->log(toSpdlogLevel(level), "[{}::{}] {}",
                            module, operation, message);
    } else if (!module.empty()) {
        m_spdlogLogger->log(toSpdlogLevel(level), "[{}] {}",
                            module, message);
    } else {
        m_spdlogLogger->log(toSpdlogLevel(level), "{}", message);
    }

    // 同时输出到旧版 ISink (兼容)
    LogRecord record;
    record.level = level;
    record.message = message;
    record.module = module;
    record.operation = operation;
    record.timestamp = Time::nowString("yyyy-MM-dd HH:mm:ss.zzz");
    m_legacySink->log(record);
}

void Logger::log(LogLevel level, const QString& message,
                 const QString& module, const QString& operation) {
    log(level, message.toStdString(), module.toStdString(), operation.toStdString());
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_spdlogLogger) {
        m_spdlogLogger->flush();
    }
    m_legacySink->flush();
}

void Logger::init() {
    // spdlog 已在构造中初始化
}

void Logger::shutdown() {
    flush();  // flush() 内部自行加锁,此处不再重复加锁避免死锁
    std::lock_guard<std::mutex> lock(m_mutex);
    spdlog::shutdown();
}

}