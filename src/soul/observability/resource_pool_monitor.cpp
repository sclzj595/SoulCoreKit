// ============================================================================
// resource_pool_monitor.cpp — 资源池监控实现
// ============================================================================
//
// 实现 IResourcePoolMonitor 接口的三个适配器(ThreadPool/DbConnectionPool/
// NetworkConnectionPool)、ResourcePoolMonitorRegistry 单例、
// ResourcePoolMetricsCollector 后台采集器。
//
// 设计原则:
//   - 严禁裸指针: 所有适配器持有引用,由调用方保证生命周期
//   - 线程安全: Registry 用 mutex 保护,Collector 用 atomic flag
//   - RAII: Collector 析构时自动 stop,避免线程泄漏

#include "soul/observability/resource_pool_monitor.h"
#include "soul/observability/metrics.h"
#include "soul/logging/log_macros.h"
#include "soul/async/thread_pool.h"
#include "soul/network/pool/connection_pool.h"
#include "soul/data/connection_pool.h"

#include <algorithm>
#include <cmath>

namespace sc {
namespace observability {

// ============================================================================
// ResourcePoolMonitorRegistry 实现
// ============================================================================

ResourcePoolMonitorRegistry& ResourcePoolMonitorRegistry::instance() {
    static ResourcePoolMonitorRegistry s_instance;
    return s_instance;
}

void ResourcePoolMonitorRegistry::registerMonitor(std::shared_ptr<IResourcePoolMonitor> monitor) {
    if (!monitor) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_monitors[monitor->name()] = std::move(monitor);
}

void ResourcePoolMonitorRegistry::unregisterMonitor(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_monitors.erase(name);
}

std::vector<std::string> ResourcePoolMonitorRegistry::names() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> result;
    result.reserve(m_monitors.size());
    for (const auto& pair : m_monitors) {
        result.push_back(pair.first);
    }
    return result;
}

std::vector<ResourcePoolSnapshot> ResourcePoolMonitorRegistry::snapshots() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<ResourcePoolSnapshot> result;
    result.reserve(m_monitors.size());
    for (const auto& pair : m_monitors) {
        if (pair.second) {
            result.push_back(pair.second->snapshot());
        }
    }
    return result;
}

void ResourcePoolMonitorRegistry::setAlertThreshold(double threshold, AlertCallback callback) {
    // 限制 threshold 在 [0.0, 1.0] 区间
    if (threshold < 0.0) threshold = 0.0;
    if (threshold > 1.0) threshold = 1.0;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_alertThreshold = threshold;
    m_alertCallback = std::move(callback);
}

void ResourcePoolMonitorRegistry::checkAlerts() const {
    AlertCallback callback;
    double threshold = 0.0;
    std::vector<ResourcePoolSnapshot> snaps;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        callback = m_alertCallback;
        threshold = m_alertThreshold;
        snaps.reserve(m_monitors.size());
        for (const auto& pair : m_monitors) {
            if (pair.second) {
                snaps.push_back(pair.second->snapshot());
            }
        }
    }
    if (!callback) {
        return;
    }
    for (const auto& s : snaps) {
        if (s.utilization > threshold) {
            callback(s.name, s.utilization);
        }
    }
}

void ResourcePoolMonitorRegistry::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_monitors.clear();
    m_alertCallback = nullptr;
}

// ============================================================================
// ThreadPoolMonitor 实现
// ============================================================================

ThreadPoolMonitor::ThreadPoolMonitor(ThreadPool& pool, std::string name)
    : m_pool(pool), m_name(std::move(name)) {}

std::string ThreadPoolMonitor::name() const {
    return m_name;
}

int ThreadPoolMonitor::activeCount() const {
    return m_pool.activeThreadCount();
}

int ThreadPoolMonitor::idleCount() const {
    const int max = m_pool.maxThreadCount();
    const int active = m_pool.activeThreadCount();
    return (max > active) ? (max - active) : 0;
}

int ThreadPoolMonitor::maxCount() const {
    return m_pool.maxThreadCount();
}

// ============================================================================
// DbConnectionPoolMonitor 实现
// ============================================================================

DbConnectionPoolMonitor::DbConnectionPoolMonitor(data::DbConnectionPool& pool,
                                                   int maxSize,
                                                   std::string name)
    : m_pool(pool), m_maxSize(maxSize), m_name(std::move(name)) {}

std::string DbConnectionPoolMonitor::name() const {
    return m_name;
}

int DbConnectionPoolMonitor::activeCount() const {
    return m_pool.getActiveConnections();
}

int DbConnectionPoolMonitor::idleCount() const {
    const int total = m_pool.getPoolSize();
    const int active = m_pool.getActiveConnections();
    return (total > active) ? (total - active) : 0;
}

int DbConnectionPoolMonitor::maxCount() const {
    return m_maxSize;
}

// ============================================================================
// NetworkConnectionPoolMonitor 实现
// ============================================================================

NetworkConnectionPoolMonitor::NetworkConnectionPoolMonitor(network::ConnectionPool& pool,
                                                              std::string name)
    : m_pool(pool), m_name(std::move(name)) {}

std::string NetworkConnectionPoolMonitor::name() const {
    return m_name;
}

int NetworkConnectionPoolMonitor::activeCount() const {
    return m_pool.activeCount();
}

int NetworkConnectionPoolMonitor::idleCount() const {
    return m_pool.idleCount();
}

int NetworkConnectionPoolMonitor::maxCount() const {
    return m_pool.maxCount();
}

// ============================================================================
// ResourcePoolMetricsCollector 实现
// ============================================================================

ResourcePoolMetricsCollector::ResourcePoolMetricsCollector(std::chrono::milliseconds interval)
    : m_interval(interval) {}

ResourcePoolMetricsCollector::~ResourcePoolMetricsCollector() {
    stop();
}

void ResourcePoolMetricsCollector::start() {
    bool expected = false;
    if (!m_running.compare_exchange_strong(expected, true)) {
        // 已在运行,幂等返回
        return;
    }
    m_stopFlag.store(false);
    // 用 mutex 串行化 m_thread 的 join/赋值,保证并发 start/stop 安全
    // (CAS 仅保证 m_running 状态转换的原子性,不保护 m_thread 对象本身)
    std::lock_guard<std::mutex> lock(m_threadMutex);
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_thread = std::thread([this]() { runLoop(); });
}

void ResourcePoolMetricsCollector::stop() {
    bool expected = true;
    if (!m_running.compare_exchange_strong(expected, false)) {
        // 未运行,幂等返回
        return;
    }
    m_stopFlag.store(true);
    // 用 mutex 串行化 m_thread 的 join,与 start() 互斥
    std::lock_guard<std::mutex> lock(m_threadMutex);
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

bool ResourcePoolMetricsCollector::isRunning() const noexcept {
    return m_running.load();
}

void ResourcePoolMetricsCollector::collectOnce() {
    auto& registry = ResourcePoolMonitorRegistry::instance();
    auto& metrics = MetricsRegistry::instance();

    auto& activeGauge = metrics.gauge("resource_pool_active_count",
                                        "Active resource count of resource pools");
    auto& idleGauge = metrics.gauge("resource_pool_idle_count",
                                      "Idle resource count of resource pools");
    auto& maxGauge = metrics.gauge("resource_pool_max_count",
                                     "Max capacity of resource pools");
    auto& utilGauge = metrics.gauge("resource_pool_utilization",
                                      "Utilization (active/max) of resource pools, [0.0, 1.0]");

    const auto snaps = registry.snapshots();
    for (const auto& s : snaps) {
        // Labels 类型为 std::map<QString, QString>(见 metrics.h)
        const Labels labels{{QString::fromStdString("name"), QString::fromStdString(s.name)}};
        activeGauge.set(labels, static_cast<double>(s.activeCount));
        idleGauge.set(labels, static_cast<double>(s.idleCount));
        maxGauge.set(labels, static_cast<double>(s.maxCount));
        utilGauge.set(labels, s.utilization);
    }

    // 检查阈值告警
    registry.checkAlerts();
}

void ResourcePoolMetricsCollector::runLoop() {
    SC_INFO("ResourcePoolMetricsCollector started, interval=" + std::to_string(m_interval.count()) + "ms");
    while (!m_stopFlag.load()) {
        collectOnce();
        // 分段睡眠,便于快速响应 stop 请求
        const auto slice = std::chrono::milliseconds(100);
        auto remaining = m_interval;
        while (remaining > slice && !m_stopFlag.load()) {
            std::this_thread::sleep_for(slice);
            remaining -= slice;
        }
        if (!m_stopFlag.load()) {
            std::this_thread::sleep_for(remaining);
        }
    }
    SC_INFO("ResourcePoolMetricsCollector stopped");
}

} // namespace observability
} // namespace sc
