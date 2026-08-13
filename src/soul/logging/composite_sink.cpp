#include "soul/logging/composite_sink.h"
#include <algorithm>

namespace sc {

void CompositeSink::addSink(std::shared_ptr<ISink> sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (sink) {
        m_sinks.push_back(sink);
    }
}

void CompositeSink::removeSink(ISink* sink) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_sinks.begin(), m_sinks.end(),
                           [sink](const std::shared_ptr<ISink>& s) { return s.get() == sink; });
    if (it != m_sinks.end()) {
        m_sinks.erase(it);
    }
}

void CompositeSink::log(const LogRecord& record) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& sink : m_sinks) {
        sink->log(record);
    }
}

void CompositeSink::flush() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& sink : m_sinks) {
        sink->flush();
    }
}

}