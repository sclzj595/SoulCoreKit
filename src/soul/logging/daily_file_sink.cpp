#include "soul/logging/daily_file_sink.h"
#include "soul/core/time.h"
#include <cstdio>
#include <filesystem>
#include <string>

namespace sc {

namespace {
// 跨平台安全打开文件（追加模式），规避 MSVC fopen C4996 警告
FILE* openAppendFile(const char* path) {
    FILE* f = nullptr;
#ifdef _WIN32
    fopen_s(&f, path, "a");
#else
    f = std::fopen(path, "a");
#endif
    return f;
}
}  // namespace

DailyFileSink::DailyFileSink(const std::string& basePath)
    : m_basePath(basePath), m_formatter(std::make_unique<DefaultLogFormatter>()) {}

DailyFileSink::DailyFileSink(const std::string& basePath, std::unique_ptr<LogFormatter> formatter)
    : m_basePath(basePath), m_formatter(std::move(formatter)) {}

void DailyFileSink::log(const LogRecord& record) {
    checkAndRotate(record.timestamp);

    if (!m_file) return;
    std::string formatted = m_formatter->format(record) + "\n";
    fwrite(formatted.c_str(), 1, formatted.size(), m_file);
}

void DailyFileSink::flush() {
    if (m_file) fflush(m_file);
}

void DailyFileSink::checkAndRotate(const std::string& timestamp) {
    std::string date = timestamp.substr(0, 10);
    if (date != m_currentDate) {
        if (m_file) {
            fclose(m_file);
            m_file = nullptr;
        }

        std::filesystem::create_directories(std::filesystem::path(m_basePath).parent_path());
        std::string filePath = m_basePath + "." + date;
        m_file = openAppendFile(filePath.c_str());
        m_currentDate = date;
    }
}

}