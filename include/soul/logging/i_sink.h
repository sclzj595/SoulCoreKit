#ifndef SOUL_LOGGING_I_SINK_H
#define SOUL_LOGGING_I_SINK_H

#include "soul/logging/log_record.h"
#include "soul/core/interface.h"

namespace sc {

/**
 * @class ISink
 * @brief 日志输出接口
 *
 * ISink 定义了日志输出的标准接口，所有日志输出目标都应实现此接口。
 * 支持自定义扩展，如控制台输出、文件输出、网络输出等。
 *
 * @see LogRecord, ConsoleSink, FileSink, CallbackSink
 */
class ISink : public IInterface {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~ISink() = default;

    /**
     * @brief 输出日志记录
     * @param record 日志记录
     */
    virtual void log(const LogRecord& record) = 0;

    /**
     * @brief 刷新缓冲区
     */
    virtual void flush() = 0;
};

}

#endif
