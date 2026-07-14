#ifndef SOUL_LOGGING_CALLBACK_SINK_H
#define SOUL_LOGGING_CALLBACK_SINK_H

#include "soul/logging/i_sink.h"
#include "soul/logging/log_formatter.h"
#include <memory>
#include <functional>

namespace sc {

/**
 * @class CallbackSink
 * @brief 回调函数日志输出目标
 *
 * CallbackSink 将日志记录通过回调函数传递给调用者，适用于：
 * - UI 界面显示日志
 * - 远程日志上报
 * - 自定义日志处理逻辑
 *
 * @see ISink, LogFormatter
 */
class CallbackSink : public ISink {
public:
    /**
     * @brief 日志回调函数类型
     */
    using LogCallback = std::function<void(const LogRecord& record)>;

    /**
     * @brief 构造函数（使用默认格式化器）
     * @param callback 日志回调函数
     */
    explicit CallbackSink(LogCallback callback);

    /**
     * @brief 构造函数（使用自定义格式化器）
     * @param callback 日志回调函数
     * @param formatter 日志格式化器
     */
    CallbackSink(LogCallback callback, std::unique_ptr<LogFormatter> formatter);

    /**
     * @brief 输出日志记录（通过回调函数）
     * @param record 日志记录
     */
    void log(const LogRecord& record) override;

    /**
     * @brief 刷新缓冲区（回调输出不需要刷新）
     */
    void flush() override;

    /**
     * @brief 获取接口名称
     * @return 接口名称
     */
    std::string interfaceName() const override { return "CallbackSink"; }

private:
    LogCallback m_callback;
    std::unique_ptr<LogFormatter> m_formatter;
};

}

#endif
