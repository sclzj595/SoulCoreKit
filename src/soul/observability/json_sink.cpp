#include "soul/observability/json_sink.h"

#include <sstream>
#include <stdexcept>
#include <iomanip>

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
    std::ostringstream oss;
    oss << '{';

    // timestamp
    oss << "\"timestamp\":\"" << escapeJson(record.timestamp) << '"';

    // level
    oss << ",\"level\":\"";
    switch (record.level) {
        case LogLevel::Trace:  oss << "TRACE";    break;
        case LogLevel::Debug:  oss << "DEBUG";    break;
        case LogLevel::Info:   oss << "INFO";     break;
        case LogLevel::Warn:   oss << "WARN";     break;
        case LogLevel::Error:  oss << "ERROR";    break;
        case LogLevel::Fatal:  oss << "FATAL";    break;
        default:               oss << "UNKNOWN";  break;
    }
    oss << '"';

    // module
    if (!record.module.empty()) {
        oss << ",\"module\":\"" << escapeJson(record.module) << '"';
    }

    // operation
    if (!record.operation.empty()) {
        oss << ",\"operation\":\"" << escapeJson(record.operation) << '"';
    }

    // message
    oss << ",\"message\":\"" << escapeJson(record.message) << '"';

    // file
    if (!record.file.empty()) {
        oss << ",\"file\":\"" << escapeJson(record.file) << '"';
    }

    // line
    oss << ",\"line\":" << record.line;

    // thread_id
    if (!record.threadId.empty()) {
        oss << ",\"thread_id\":\"" << escapeJson(record.threadId) << '"';
    }

    // process_id
    if (!record.processId.empty()) {
        oss << ",\"process_id\":\"" << escapeJson(record.processId) << '"';
    }

    oss << '}';
    return oss.str();
}

std::string JsonSink::escapeJson(const std::string& s) {
    std::ostringstream oss;
    for (char c : s) {
        switch (c) {
            case '"':  oss << "\\\""; break;
            case '\\': oss << "\\\\"; break;
            case '\b': oss << "\\b";  break;
            case '\f': oss << "\\f";  break;
            case '\n': oss << "\\n";  break;
            case '\r': oss << "\\r";  break;
            case '\t': oss << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    // 控制字符转义为 \uXXXX
                    oss << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                        << static_cast<int>(static_cast<unsigned char>(c));
                } else {
                    oss << c;
                }
                break;
        }
    }
    return oss.str();
}

} // namespace observability
} // namespace sc
