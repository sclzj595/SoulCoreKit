#include "soul/logging/log_formatter.h"

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

    if (record.module.empty() && record.operation.empty()) {
        return "[" + record.timestamp + "][" + levelStr + "] " + record.message;
    }
    return "[" + record.timestamp + "][" + levelStr + "][" + record.module + "][" + record.operation + "] " + record.message;
}

}