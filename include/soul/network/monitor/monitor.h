#ifndef SOUL_NETWORK_MONITOR_MONITOR_H
#define SOUL_NETWORK_MONITOR_MONITOR_H

#include <memory>
#include <QObject>
#include "metrics.h"

namespace sc::network {

class Monitor : public QObject {
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

}

#endif