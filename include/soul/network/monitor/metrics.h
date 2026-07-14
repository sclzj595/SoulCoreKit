#ifndef SOUL_NETWORK_MONITOR_METRICS_H
#define SOUL_NETWORK_MONITOR_METRICS_H

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <QDateTime>

namespace sc::network {

struct MetricData {
    int64_t qps{0};
    int64_t totalRequests{0};
    int64_t successRequests{0};
    int64_t failedRequests{0};
    int64_t retryCount{0};
    int64_t reconnectCount{0};
    int64_t timeoutCount{0};
    int64_t bytesSent{0};
    int64_t bytesReceived{0};
    int64_t totalResponseTime{0};
    int64_t minResponseTime{INT64_MAX};
    int64_t maxResponseTime{0};
    
    void reset();
};

class Metrics : public std::enable_shared_from_this<Metrics> {
public:
    Metrics();
    
    void recordRequest();
    void recordSuccess(int64_t responseTimeMs);
    void recordFailure();
    void recordRetry();
    void recordReconnect();
    void recordTimeout();
    void recordBytesSent(size_t bytes);
    void recordBytesReceived(size_t bytes);
    
    MetricData snapshot() const;
    
    double qps() const;
    double successRate() const;
    double avgResponseTime() const;
    int64_t minResponseTime() const;
    int64_t maxResponseTime() const;
    
private:
    mutable std::mutex m_mutex;
    MetricData m_data;
    std::chrono::steady_clock::time_point m_startTime;
    int64_t m_qpsWindowStart{0};
    int64_t m_qpsCount{0};
};

}

#endif