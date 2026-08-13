// ============================================================================
// health.cpp — Health 实现 [v2.8.0]
// ============================================================================

#include "soul/core/health.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#include <sys/resource.h>
#endif

namespace sc {

// ============================================================================
// HealthAggregator 实现
// ============================================================================

HealthAggregator& HealthAggregator::instance() {
    static HealthAggregator inst;
    return inst;
}

void HealthAggregator::registerIndicator(std::shared_ptr<IHealthIndicator> indicator) {
    if (!indicator) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_indicators.push_back(std::move(indicator));
}

void HealthAggregator::unregisterIndicator(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_indicators.erase(
        std::remove_if(m_indicators.begin(), m_indicators.end(),
            [&name](const auto& ind) { return ind->name() == name; }),
        m_indicators.end());
}

std::vector<HealthStatus> HealthAggregator::checkAll() {
    // 快照模式: 避免持锁调用 check()
    std::vector<std::shared_ptr<IHealthIndicator>> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        snapshot = m_indicators;
    }

    std::vector<HealthStatus> results;
    results.reserve(snapshot.size());
    for (auto& ind : snapshot) {
        results.push_back(ind->check());
    }
    return results;
}

HealthStatusCode HealthAggregator::overallStatus() {
    auto results = checkAll();
    if (results.empty()) return HealthStatusCode::Up;

    bool hasDown = false;
    bool hasDegraded = false;
    for (auto& r : results) {
        if (r.code == HealthStatusCode::Down) hasDown = true;
        if (r.code == HealthStatusCode::Degraded) hasDegraded = true;
    }

    if (hasDown) return HealthStatusCode::Down;
    if (hasDegraded) return HealthStatusCode::Degraded;
    return HealthStatusCode::Up;
}

bool HealthAggregator::isHealthy() {
    return overallStatus() == HealthStatusCode::Up;
}

bool HealthAggregator::isReady() {
    auto status = overallStatus();
    return status != HealthStatusCode::Down;
}

QByteArray HealthAggregator::toJson() {
    auto results = checkAll();

    QJsonObject root;
    root["status"] = (overallStatus() == HealthStatusCode::Up) ? "UP" :
                     (overallStatus() == HealthStatusCode::Down) ? "DOWN" : "DEGRADED";

    QJsonObject deps;
    for (auto& r : results) {
        QJsonObject dep;
        dep["status"] = r.isUp() ? "UP" : (r.isDown() ? "DOWN" : "DEGRADED");
        if (!r.message.isEmpty()) {
            dep["message"] = r.message;
        }
        if (!r.details.isEmpty()) {
            dep["details"] = r.details;
        }
        deps[r.name] = dep;
    }
    root["dependencies"] = deps;

    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

// ============================================================================
// MemoryHealthIndicator 实现
// ============================================================================

MemoryHealthIndicator::MemoryHealthIndicator(uint64_t maxUsageMB)
    : m_maxUsageMB(maxUsageMB) {}

uint64_t MemoryHealthIndicator::currentMemoryUsageMB() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX pmc{};
    pmc.cb = sizeof(pmc);
    if (GetProcessMemoryInfo(GetCurrentProcess(),
                             reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pmc),
                             sizeof(pmc))) {
        return pmc.PrivateUsage / (1024 * 1024);
    }
    return 0;
#else
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        return usage.ru_maxrss / 1024;  // ru_maxrss 单位 KB
    }
    return 0;
#endif
}

HealthStatus MemoryHealthIndicator::check() {
    uint64_t memMB = currentMemoryUsageMB();

    HealthStatus status;
    status.name = "memory";
    status.details["usage_mb"] = static_cast<double>(memMB);

    if (m_maxUsageMB > 0 && memMB > m_maxUsageMB) {
        status.code = HealthStatusCode::Degraded;
        status.message = QString("Memory usage %1MB exceeds threshold %2MB")
            .arg(memMB).arg(m_maxUsageMB);
    } else {
        status.code = HealthStatusCode::Up;
        status.message = "OK";
    }
    return status;
}

} // namespace sc
