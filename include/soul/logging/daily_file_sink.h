#ifndef SOUL_LOGGING_DAILY_FILE_SINK_H
#define SOUL_LOGGING_DAILY_FILE_SINK_H

#include "soul/logging/i_sink.h"
#include "soul/logging/log_formatter.h"
#include <memory>
#include <string>

namespace sc {

/**
 * @class DailyFileSink
 * @brief 文件日志输出目标，按天滚动
 *
 * DailyFileSink 将日志输出到文件，每天自动创建新的日志文件。
 * 文件命名格式：basePath_YYYY-MM-DD.log
 *
 * @see ISink, LogFormatter, FileSink
 */
class DailyFileSink : public ISink {
public:
    /**
     * @brief 构造函数（使用默认格式化器）
     * @param basePath 日志文件基础路径
     */
    explicit DailyFileSink(const std::string& basePath);

    /**
     * @brief 构造函数（使用自定义格式化器）
     * @param basePath 日志文件基础路径
     * @param formatter 日志格式化器
     */
    DailyFileSink(const std::string& basePath, std::unique_ptr<LogFormatter> formatter);

    /**
     * @brief 输出日志记录
     * @param record 日志记录
     */
    void log(const LogRecord& record) override;

    /**
     * @brief 刷新缓冲区
     */
    void flush() override;

    /**
     * @brief 获取接口名称
     * @return 接口名称
     */
    std::string interfaceName() const override { return "DailyFileSink"; }

private:
    /**
     * @brief 检查并滚动日志文件（根据日期）
     * @param timestamp 时间戳
     */
    void checkAndRotate(const std::string& timestamp);

    std::string m_basePath;
    std::unique_ptr<LogFormatter> m_formatter;
    FILE* m_file = nullptr;
    std::string m_currentDate;
};

}

#endif
