#include <memory>
#include <mutex>
#include <string>
#include "soul/logging/logger.h"
#include "soul/logging/console_sink.h"
#include "soul/core/time.h"

namespace sc {

Logger::Logger() : m_sink(std::make_shared<CompositeSink>()) {
    m_sink->addSink(std::make_shared<ConsoleSink>());
}

Logger& Logger::instance() {
    static Logger inst;
    return inst;
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

LogLevel Logger::logLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

// ============================================================================
// 模块级日志级别 [v1.9.2 新增]
// ============================================================================

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
    return m_level;  // 未设置时返回全局级别
}

void Logger::removeModuleLogLevel(const std::string& module) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_moduleLevels.erase(module);
}

void Logger::addSink(std::shared_ptr<ISink> sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sink->addSink(sink);
}

void Logger::removeSink(ISink* sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sink->removeSink(sink);
}

void Logger::log(LogLevel level, const std::string& message) {
    log(level, message, "", "");
}

void Logger::log(LogLevel level, const QString& message) {
    log(level, message.toStdString());
}

void Logger::log(LogLevel level, const std::string& message,
                 const std::string& module, const std::string& operation) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // [v1.9.2] 模块级日志过滤: 先检查模块级别,再检查全局级别
    if (!module.empty()) {
        auto it = m_moduleLevels.find(module);
        if (it != m_moduleLevels.end()) {
            if (level < it->second) return;
        } else if (level < m_level) {
            return;
        }
    } else if (level < m_level) {
        return;
    }

    LogRecord record;
    record.level = level;
    record.message = message;
    record.module = module;
    record.operation = operation;
    record.timestamp = Time::nowString("yyyy-MM-dd HH:mm:ss.zzz");

    m_sink->log(record);
}

void Logger::log(LogLevel level, const QString& message,
                 const QString& module, const QString& operation) {
    log(level, message.toStdString(), module.toStdString(), operation.toStdString());
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_sink->flush();
}

void Logger::init() {
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    flush();
}

}