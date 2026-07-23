#include "soul/logging/log_formatter.h"
#include <sstream>
#include <string>

namespace sc {

std::string DefaultLogFormatter::format(const LogRecord& record) {
    std::string levelStr;
    switch (record.level) {
    case LogLevel::Trace: levelStr = "TRACE"; break;
    case LogLevel::Debug: levelStr = "DEBUG"; break;
    case LogLevel::Info:  levelStr = "INFO";  break;
    case LogLevel::Warn:  levelStr = "WARN";  break;
    case LogLevel::Error: levelStr = "ERROR"; break;
    case LogLevel::Fatal: levelStr = "FATAL"; break;
    }

    std::ostringstream oss;
    oss << "[" << record.timestamp << "]";
    
    if (!record.processId.empty()) {
        oss << "[PID:" << record.processId << "]";
    }
    
    if (!record.threadId.empty()) {
        oss << "[TID:" << record.threadId << "]";
    }
    
    oss << "[" << levelStr << "]";
    
    if (!record.module.empty()) {
        oss << "[" << record.module << "]";
    }
    
    if (!record.operation.empty()) {
        oss << "[" << record.operation << "]";
    }
    
    oss << " " << record.message;
    
    return oss.str();
}

std::string JsonLogFormatter::format(const LogRecord& record) {
    std::string levelStr;
    switch (record.level) {
    case LogLevel::Trace: levelStr = "\"trace\""; break;
    case LogLevel::Debug: levelStr = "\"debug\""; break;
    case LogLevel::Info:  levelStr = "\"info\"";  break;
    case LogLevel::Warn:  levelStr = "\"warn\"";  break;
    case LogLevel::Error: levelStr = "\"error\""; break;
    case LogLevel::Fatal: levelStr = "\"fatal\""; break;
    }

    std::ostringstream oss;
    oss << "{";
    oss << "\"timestamp\":\"" << record.timestamp << "\",";
    oss << "\"level\":" << levelStr << ",";
    oss << "\"module\":\"" << record.module << "\",";
    oss << "\"operation\":\"" << record.operation << "\",";
    oss << "\"message\":\"" << record.message << "\",";
    oss << "\"file\":\"" << record.file << "\",";
    oss << "\"line\":" << record.line << ",";
    oss << "\"thread_id\":\"" << record.threadId << "\",";
    oss << "\"process_id\":\"" << record.processId << "\"";
    oss << "}";

    return oss.str();
}

}