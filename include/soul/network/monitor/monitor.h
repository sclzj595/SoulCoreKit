/**
 * @file monitor/monitor.h
 * @brief 网络监控类
 * @details 监控网络请求的性能指标和统计信息
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_MONITOR_MONITOR_H
#define SOUL_NETWORK_MONITOR_MONITOR_H

#include <memory>
#include <mutex>
#include <unordered_map>
#include <QObject>
#include "soul/network/monitor/metrics.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT Monitor : public QObject {
    Q_OBJECT
public:
    explicit Monitor(QObject* parent = nullptr);
    
    std::shared_ptr<Metrics> getMetrics(const QString& key);
    void registerMetrics(const QString& key, std::shared_ptr<Metrics> metrics);
    
    void report();
    
signals:
    void metricsUpdated(const QString& key, const MetricData& data);
    
private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::shared_ptr<Metrics>> m_metricsMap;
};

} // namespace network
} // namespace sc

#endif