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
    virtual ~ISink() override = default;

    virtual void log(const LogRecord& record) = 0;

    virtual void flush() = 0;

    std::string interfaceName() const override {
        return "ISink";
    }
};

}

#endif
