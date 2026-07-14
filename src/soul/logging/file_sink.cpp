#include "soul/logging/file_sink.h"
#include <cstdio>
#include <filesystem>

namespace sc {

FileSink::FileSink(const std::string& filePath, size_t maxSize, int maxFiles)
    : m_filePath(filePath), m_maxSize(maxSize), m_maxFiles(maxFiles),
      m_formatter(std::make_unique<DefaultLogFormatter>()) {
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
    m_file = fopen(filePath.c_str(), "a");
}

FileSink::FileSink(const std::string& filePath, std::unique_ptr<LogFormatter> formatter,
                   size_t maxSize, int maxFiles)
    : m_filePath(filePath), m_maxSize(maxSize), m_maxFiles(maxFiles),
      m_formatter(std::move(formatter)) {
    std::filesystem::create_directories(std::filesystem::path(filePath).parent_path());
    m_file = fopen(filePath.c_str(), "a");
}

void FileSink::log(const LogRecord& record) {
    if (!m_file) return;

    std::string formatted = m_formatter->format(record) + "\n";
    size_t len = formatted.size();

    if (m_currentSize + len > m_maxSize) {
        rotateFile();
    }

    fwrite(formatted.c_str(), 1, len, m_file);
    m_currentSize += len;
}

void FileSink::flush() {
    if (m_file) fflush(m_file);
}

void FileSink::rotateFile() {
    if (m_file) {
        fclose(m_file);
        m_file = nullptr;
    }

    for (int i = m_maxFiles - 1; i > 0; i--) {
        std::string src = m_filePath + "." + std::to_string(i);
        std::string dst = m_filePath + "." + std::to_string(i + 1);
        if (std::filesystem::exists(src)) {
            std::filesystem::rename(src, dst);
        }
    }

    std::filesystem::rename(m_filePath, m_filePath + ".1");
    m_file = fopen(m_filePath.c_str(), "a");
    m_currentSize = 0;
}

}