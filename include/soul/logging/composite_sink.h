#ifndef SOUL_LOGGING_COMPOSITE_SINK_H
#define SOUL_LOGGING_COMPOSITE_SINK_H

#include "soul/logging/i_sink.h"
#include <vector>
#include <memory>

namespace sc {

/**
 * @class CompositeSink
 * @brief 组合日志输出目标，聚合多个Sink
 *
 * CompositeSink 将多个日志输出目标组合在一起，调用一次 log() 方法
 * 会将日志记录发送到所有已添加的 Sink。
 *
 * @see ISink, ConsoleSink, FileSink
 */
class CompositeSink : public ISink {
public:
    /**
     * @brief 添加日志输出目标
     * @param sink 日志输出目标
     */
    void addSink(std::shared_ptr<ISink> sink);

    /**
     * @brief 移除日志输出目标
     * @param sink 日志输出目标指针
     */
    void removeSink(ISink* sink);

    /**
     * @brief 输出日志记录（发送到所有子Sink）
     * @param record 日志记录
     */
    void log(const LogRecord& record) override;

    /**
     * @brief 刷新所有子Sink的缓冲区
     */
    void flush() override;

    /**
     * @brief 获取接口名称
     * @return 接口名称
     */
    std::string interfaceName() const override { return "CompositeSink"; }

private:
    std::vector<std::shared_ptr<ISink>> m_sinks;
};

}

#endif
