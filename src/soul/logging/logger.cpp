#include <memory>
#include "soul/logging/logger.h"
#include "soul/logging/console_sink.h"
#include "soul/core/time.h"
#include "soul/core/singleton.h"

namespace sc {

Logger::Logger() : m_sink(std::make_shared<CompositeSink>()) {
    m_sink->addSink(std::make_shared<ConsoleSink>());
    SingletonRegistry::instance().registerShutdown([this]() {
        this->shutdown();
    });
}

void Logger::setLogLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

LogLevel Logger::logLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
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

void Logger::log(LogLevel level, const std::string& message,
                 const std::string& module, const std::string& operation) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (level < m_level) return;

    LogRecord record;
    record.level = level;
    record.message = message;
    record.module = module;
    record.operation = operation;
    record.timestamp = Time::nowString("yyyy-MM-dd HH:mm:ss.zzz");

    m_sink->log(record);
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