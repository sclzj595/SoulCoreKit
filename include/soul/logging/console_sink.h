#ifndef SOUL_LOGGING_CONSOLE_SINK_H
#define SOUL_LOGGING_CONSOLE_SINK_H

#include "soul/logging/i_sink.h"
#include "soul/logging/log_formatter.h"
#include <memory>

namespace sc {

/**
 * @class ConsoleSink
 * @brief 控制台日志输出目标
 *
 * ConsoleSink 将日志输出到标准输出（stdout），支持彩色输出（如果终端支持）。
 * 默认使用 DefaultLogFormatter 进行格式化。
 *
 * @see ISink, LogFormatter
 */
class ConsoleSink : public ISink {
public:
    /**
     * @brief 构造函数（使用默认格式化器）
     */
    ConsoleSink();

    /**
     * @brief 构造函数（使用自定义格式化器）
     * @param formatter 日志格式化器
     */
    explicit ConsoleSink(std::unique_ptr<LogFormatter> formatter);

    /**
     * @brief 输出日志记录
     * @param record 日志记录
     */
    void log(const LogRecord& record) override;

    /**
     * @brief 刷新缓冲区（控制台输出通常不需要刷新）
     */
    void flush() override;

    /**
     * @brief 获取接口名称
     * @return 接口名称
     */
    std::string interfaceName() const override { return "ConsoleSink"; }

private:
    std::unique_ptr<LogFormatter> m_formatter;
};

}

#endif
