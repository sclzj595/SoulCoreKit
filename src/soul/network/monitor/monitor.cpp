#include <memory>
#include <mutex>
#include "soul/network/monitor/monitor.h"

namespace sc {
namespace network {

Monitor::Monitor(QObject* parent)
    : QObject(parent) {
}

std::shared_ptr<Metrics> Monitor::getMetrics(const QString& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_metricsMap.find(key.toStdString());
    if (it != m_metricsMap.end()) {
        return it->second;
    }
    
    auto metrics = std::make_shared<Metrics>();
    m_metricsMap[key.toStdString()] = metrics;
    return metrics;
}

void Monitor::registerMetrics(const QString& key, std::shared_ptr<Metrics> metrics) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metricsMap[key.toStdString()] = metrics;
}

void Monitor::report() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& pair : m_metricsMap) {
        MetricData data = pair.second->snapshot();
        emit metricsUpdated(QString::fromStdString(pair.first), data);
    }
}

} // namespace network
} // namespace sc