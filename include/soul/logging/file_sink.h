#ifndef SOUL_LOGGING_FILE_SINK_H
#define SOUL_LOGGING_FILE_SINK_H

#include "soul/logging/i_sink.h"
#include "soul/logging/log_formatter.h"
#include <memory>
#include <string>

namespace sc {

/**
 * @class FileSink
 * @brief 文件日志输出目标，支持大小滚动
 *
 * FileSink 将日志输出到文件，当文件大小超过指定限制时自动滚动。
 * 支持配置最大文件大小和最大保留文件数。
 *
 * @see ISink, LogFormatter, DailyFileSink
 */
class FileSink : public ISink {
public:
    /**
     * @brief 构造函数（使用默认格式化器）
     * @param filePath 日志文件路径
     * @param maxSize 最大文件大小（字节），默认 10MB
     * @param maxFiles 最大保留文件数，默认 5
     */
    explicit FileSink(const std::string& filePath, size_t maxSize = 10 * 1024 * 1024, int maxFiles = 5);

    /**
     * @brief 构造函数（使用自定义格式化器）
     * @param filePath 日志文件路径
     * @param formatter 日志格式化器
     * @param maxSize 最大文件大小（字节），默认 10MB
     * @param maxFiles 最大保留文件数，默认 5
     */
    FileSink(const std::string& filePath, std::unique_ptr<LogFormatter> formatter,
             size_t maxSize = 10 * 1024 * 1024, int maxFiles = 5);

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
    std::string interfaceName() const override { return "FileSink"; }

private:
    /**
     * @brief 滚动日志文件
     */
    void rotateFile();

    std::string m_filePath;
    size_t m_maxSize;
    int m_maxFiles;
    std::unique_ptr<LogFormatter> m_formatter;
    FILE* m_file = nullptr;
    size_t m_currentSize = 0;
};

}

#endif
