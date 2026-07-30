#ifndef SOUL_OBSERVABILITY_JSON_SINK_H
#define SOUL_OBSERVABILITY_JSON_SINK_H

#include <QString>
#include <fstream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include "soul/logging/i_sink.h"
#include "soul/logging/log_record.h"
#include "soul/logging/log_level.h"

namespace sc {
namespace observability {

/**
 * @class JsonSink
 * @brief 结构化 JSON 日志输出
 *
 * 继承 logging::ISink，将 LogRecord 序列化为单行 JSON（JSONL）格式输出。
 * 适合被日志聚合系统（ELK、Loki、Splunk 等）消费。
 *
 * @par 输出格式（单行 JSON）
 * @code
 * {"timestamp":"2026-07-26T10:30:45.123Z","level":"INFO","module":"network","operation":"connect","message":"Connected","file":"client.cpp","line":42,"thread_id":"0x7f8a","process_id":"12345"}
 * @endcode
 *
 * @par 使用示例
 * @code
 * auto jsonSink = std::make_shared<JsonSink>("app.log.jsonl");
 * Logger::instance().addSink(jsonSink);
 * @endcode
 *
 * @thread_safety Thread-Safe — 内部加锁保护文件写入
 */
class JsonSink : public ISink {
public:
    /**
     * @brief 构造文件 JSON Sink
     * @param filePath 输出文件路径（追加模式）
     * @throws std::runtime_error 若文件无法打开
     */
    explicit JsonSink(const QString& filePath);

    /**
     * @brief 构造流式 JSON Sink（用于测试或自定义输出）
     * @param stream 输出流（生命周期由调用方管理）
     */
    explicit JsonSink(std::ostream& stream);

    ~JsonSink() override;

    /// @brief 写入一条日志记录
    void log(const LogRecord& record) override;

    /// @brief 刷新输出流
    void flush() override;

    std::string interfaceName() const override {
        return "JsonSink";
    }

private:
    /// @brief 将 LogRecord 序列化为 JSON 字符串
    [[nodiscard]] static std::string serialize(const LogRecord& record);

    std::ofstream    m_fileStream;
    std::ostream&    m_stream;
    std::mutex       m_mutex;
    bool             m_ownsStream;
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_JSON_SINK_H
