#ifndef SOUL_LOGGING_LOG_RECORD_H
#define SOUL_LOGGING_LOG_RECORD_H

#include "soul/logging/log_level.h"
#include <string>

namespace sc {

/**
 * @struct LogRecord
 * @brief 日志记录结构体，包含一条日志的所有信息
 *
 * LogRecord 封装了一条日志记录的完整信息，包括级别、模块、操作、
 * 消息、时间戳、文件、行号、线程ID和进程ID。
 */
struct LogRecord {
    LogLevel level;       ///< 日志级别
    std::string module;   ///< 模块名称
    std::string operation; ///< 操作名称
    std::string message;  ///< 日志消息
    std::string timestamp; ///< 时间戳字符串
    std::string file;     ///< 源文件路径
    int line = 0;         ///< 源文件行号
    std::string threadId; ///< 线程ID
    std::string processId; ///< 进程ID
};

}

#endif
