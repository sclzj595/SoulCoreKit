#include "soul/network/monitor/metrics.h"

namespace sc::network {

void MetricData::reset() {
    qps = 0;
    totalRequests = 0;
    successRequests = 0;
    failedRequests = 0;
    retryCount = 0;
    reconnectCount = 0;
    timeoutCount = 0;
    bytesSent = 0;
    bytesReceived = 0;
    totalResponseTime = 0;
    minResponseTime = INT64_MAX;
    maxResponseTime = 0;
}

Metrics::Metrics()
    : m_startTime(std::chrono::steady_clock::now()) {
}

void Metrics::recordRequest() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.totalRequests++;
    
    auto now = std::chrono::steady_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    if (nowMs - m_qpsWindowStart >= 1000) {
        m_qpsWindowStart = nowMs;
        m_qpsCount = 0;
    }
    m_qpsCount++;
}

void Metrics::recordSuccess(int64_t responseTimeMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.successRequests++;
    m_data.totalResponseTime += responseTimeMs;
    
    if (m_data.minResponseTime == 0 || responseTimeMs < m_data.minResponseTime) {
        m_data.minResponseTime = responseTimeMs;
    }
    if (responseTimeMs > m_data.maxResponseTime) {
        m_data.maxResponseTime = responseTimeMs;
    }
}

void Metrics::recordFailure() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.failedRequests++;
}

void Metrics::recordRetry() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.retryCount++;
}

void Metrics::recordReconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.reconnectCount++;
}

void Metrics::recordTimeout() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.timeoutCount++;
}

void Metrics::recordBytesSent(size_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.bytesSent += static_cast<int64_t>(bytes);
}

void Metrics::recordBytesReceived(size_t bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_data.bytesReceived += static_cast<int64_t>(bytes);
}

MetricData Metrics::snapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data;
}

double Metrics::qps() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto now = std::chrono::steady_clock::now();
    auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    
    if (nowMs - m_qpsWindowStart >= 1000) {
        return m_qpsCount;
    }
    return m_qpsCount;
}

double Metrics::successRate() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int64_t total = m_data.totalRequests;
    if (total == 0) return 100.0;
    return (m_data.successRequests * 100.0) / total;
}

double Metrics::avgResponseTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    int64_t success = m_data.successRequests;
    if (success == 0) return 0.0;
    return static_cast<double>(m_data.totalResponseTime) / success;
}

int64_t Metrics::minResponseTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data.minResponseTime == INT64_MAX ? 0 : m_data.minResponseTime;
}

int64_t Metrics::maxResponseTime() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_data.maxResponseTime;
}

}