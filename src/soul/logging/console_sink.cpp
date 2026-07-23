#include <string>
#include "soul/logging/console_sink.h"

namespace sc {

ConsoleSink::ConsoleSink() : m_formatter(std::make_unique<DefaultLogFormatter>()) {}

ConsoleSink::ConsoleSink(std::unique_ptr<LogFormatter> formatter)
    : m_formatter(std::move(formatter)) {}

void ConsoleSink::log(const LogRecord& record) {
    std::string formatted = m_formatter->format(record);
    switch (record.level) {
    case LogLevel::Trace:
    case LogLevel::Debug: fprintf(stdout, "%s\n", formatted.c_str()); break;
    case LogLevel::Info:  fprintf(stdout, "%s\n", formatted.c_str()); break;
    case LogLevel::Warn:  fprintf(stderr, "%s\n", formatted.c_str()); break;
    case LogLevel::Error: fprintf(stderr, "%s\n", formatted.c_str()); break;
    case LogLevel::Fatal: fprintf(stderr, "%s\n", formatted.c_str()); break;
    }
}

void ConsoleSink::flush() {
    fflush(stdout);
    fflush(stderr);
}

}