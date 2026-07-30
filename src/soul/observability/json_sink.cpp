#include "soul/observability/json_sink.h"
#include "soul/utils/json/json_helper.h"

#include <stdexcept>

namespace sc {
namespace observability {

JsonSink::JsonSink(const QString& filePath)
    : m_stream(m_fileStream)
    , m_ownsStream(true) {
    m_fileStream.open(filePath.toStdString(), std::ios::app);
    if (!m_fileStream.is_open()) {
        throw std::runtime_error("JsonSink: cannot open file: " + filePath.toStdString());
    }
}

JsonSink::JsonSink(std::ostream& stream)
    : m_stream(stream)
    , m_ownsStream(false) {
}

JsonSink::~JsonSink() {
    if (m_ownsStream && m_fileStream.is_open()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_fileStream.flush();
        m_fileStream.close();
    }
}

void JsonSink::log(const LogRecord& record) {
    std::string json = serialize(record);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stream << json << '\n';
}

void JsonSink::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stream.flush();
}

std::string JsonSink::serialize(const LogRecord& record) {
    sc::json::Json j;

    j["timestamp"] = record.timestamp;

    switch (record.level) {
        case LogLevel::Trace:  j["level"] = "TRACE";  break;
        case LogLevel::Debug:  j["level"] = "DEBUG";  break;
        case LogLevel::Info:   j["level"] = "INFO";   break;
        case LogLevel::Warn:   j["level"] = "WARN";   break;
        case LogLevel::Error:  j["level"] = "ERROR";  break;
        case LogLevel::Fatal:  j["level"] = "FATAL";  break;
        default:               j["level"] = "UNKNOWN"; break;
    }

    if (!record.module.empty()) {
        j["module"] = record.module;
    }
    if (!record.operation.empty()) {
        j["operation"] = record.operation;
    }

    j["message"] = record.message;

    if (!record.file.empty()) {
        j["file"] = record.file;
    }

    j["line"] = record.line;

    if (!record.threadId.empty()) {
        j["thread_id"] = record.threadId;
    }
    if (!record.processId.empty()) {
        j["process_id"] = record.processId;
    }

    return j.dump();
}

} // namespace observability
} // namespace sc
