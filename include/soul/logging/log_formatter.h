#ifndef SOUL_LOGGING_LOG_FORMATTER_H
#define SOUL_LOGGING_LOG_FORMATTER_H

#include "soul/logging/log_record.h"
#include <string>

namespace sc {

/**
 * @class LogFormatter
 * @brief 日志格式化器接口
 *
 * LogFormatter 定义了日志格式化的标准接口，负责将 LogRecord 转换为字符串。
 * 支持自定义格式化规则。
 *
 * @see LogRecord, DefaultLogFormatter, JsonLogFormatter
 */
class LogFormatter {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~LogFormatter() = default;

    /**
     * @brief 格式化日志记录
     * @param record 日志记录
     * @return 格式化后的字符串
     */
    virtual std::string format(const LogRecord& record) = 0;
};

/**
 * @class DefaultLogFormatter
 * @brief 默认日志格式化器
 *
 * DefaultLogFormatter 提供默认的日志格式：
 * [时间戳] [PID:进程ID] [TID:线程ID] [级别] [模块] [操作] 消息
 *
 * @see LogFormatter
 */
class DefaultLogFormatter : public LogFormatter {
public:
    /**
     * @brief 格式化日志记录
     * @param record 日志记录
     * @return 格式化后的字符串
     */
    std::string format(const LogRecord& record) override;
};

/**
 * @class JsonLogFormatter
 * @brief JSON结构化日志格式化器
 *
 * JsonLogFormatter 输出结构化的JSON格式日志，便于日志收集和分析：
 * {
 *   "timestamp": "2024-01-01 12:00:00.000",
 *   "level": "INFO",
 *   "module": "Network",
 *   "operation": "request",
 *   "message": "Request completed",
 *   "file": "network.cpp",
 *   "line": 42,
 *   "thread_id": "0x12345678",
 *   "process_id": "1234"
 * }
 *
 * @see LogFormatter
 */
class JsonLogFormatter : public LogFormatter {
public:
    /**
     * @brief 格式化日志记录
     * @param record 日志记录
     * @return 格式化后的JSON字符串
     */
    std::string format(const LogRecord& record) override;
};

}

#endif
