#include "soul/logging/callback_sink.h"

namespace sc {

CallbackSink::CallbackSink(LogCallback callback)
    : m_callback(std::move(callback)), m_formatter(std::make_unique<DefaultLogFormatter>()) {}

CallbackSink::CallbackSink(LogCallback callback, std::unique_ptr<LogFormatter> formatter)
    : m_callback(std::move(callback)), m_formatter(std::move(formatter)) {}

void CallbackSink::log(const LogRecord& record) {
    if (m_callback) {
        m_callback(record);
    }
}

void CallbackSink::flush() {}

}