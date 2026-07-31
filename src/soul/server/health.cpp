// ============================================================================
// health.cpp — Server 端健康检查端点实现
// ============================================================================

#include "soul/server/health.h"

#include <algorithm>
#include <sstream>

namespace sc {
namespace server {

// ============================================================================
// HealthEndpoint 实现
// ============================================================================

void HealthEndpoint::addIndicator(std::shared_ptr<IHealthIndicator> indicator) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_indicators[indicator->name()] = std::move(indicator);
}

void HealthEndpoint::removeIndicator(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_indicators.erase(name);
}

std::vector<std::string> HealthEndpoint::indicatorNames() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> names;
    names.reserve(m_indicators.size());
    for (const auto& pair : m_indicators) {
        names.push_back(pair.first);
    }
    return names;
}

HealthReport HealthEndpoint::readiness() const {
    return doCheck(false);
}

HealthReport HealthEndpoint::liveness() const {
    return doCheck(true);
}

void HealthEndpoint::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_indicators.clear();
}

HealthReport HealthEndpoint::doCheck(bool criticalOnly) const {
    HealthReport report;
    report.overall = HealthStatus::UNKNOWN;

    auto startTime = std::chrono::steady_clock::now();

    // 获取指示器快照
    std::vector<std::shared_ptr<IHealthIndicator>> snapshot;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& pair : m_indicators) {
            if (!criticalOnly || pair.second->isCritical()) {
                snapshot.push_back(pair.second);
            }
        }
    }

    if (snapshot.empty()) {
        report.overall = HealthStatus::UP;
        report.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
        return report;
    }

    bool hasDown = false;
    bool hasUp = false;

    for (const auto& indicator : snapshot) {
        try {
            HealthDetail detail = indicator->check();
            report.details.push_back(detail);

            if (detail.status == HealthStatus::DOWN) {
                hasDown = true;
            } else if (detail.status == HealthStatus::UP) {
                hasUp = true;
            }
        } catch (const std::exception& e) {
            HealthDetail detail;
            detail.name = indicator->name();
            detail.status = HealthStatus::DOWN;
            detail.message = std::string("Exception: ") + e.what();
            report.details.push_back(detail);
            hasDown = true;
        } catch (...) { // Blanket catch: capture unknown exception in health check
            HealthDetail detail;
            detail.name = indicator->name();
            detail.status = HealthStatus::DOWN;
            detail.message = "Unknown exception";
            report.details.push_back(detail);
            hasDown = true;
        }
    }

    // 整体状态: 任一关键依赖 DOWN → DOWN; 全部 UP → UP; 否则 UNKNOWN
    if (hasDown) {
        report.overall = HealthStatus::DOWN;
    } else if (hasUp) {
        report.overall = HealthStatus::UP;
    }

    report.totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime);
    return report;
}

// ============================================================================
// DatabaseHealthIndicator 实现
// ============================================================================

DatabaseHealthIndicator::DatabaseHealthIndicator(CheckFunc checkFunc, std::string name)
    : m_checkFunc(std::move(checkFunc)), m_name(std::move(name)) {
}

HealthDetail DatabaseHealthIndicator::check() const {
    HealthDetail detail;
    detail.name = m_name;

    auto start = std::chrono::steady_clock::now();
    try {
        if (m_checkFunc()) {
            detail.status = HealthStatus::UP;
            detail.message = "Database connection is healthy";
        } else {
            detail.status = HealthStatus::DOWN;
            detail.message = "Database connection check failed";
        }
    } catch (const std::exception& e) {
        detail.status = HealthStatus::DOWN;
        detail.message = std::string("Database check exception: ") + e.what();
    } catch (...) {
        detail.status = HealthStatus::DOWN;
        detail.message = "Database check unknown exception";
    }
    detail.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    return detail;
}

// ============================================================================
// MqHealthIndicator 实现
// ============================================================================

MqHealthIndicator::MqHealthIndicator(CheckFunc checkFunc, std::string name)
    : m_checkFunc(std::move(checkFunc)), m_name(std::move(name)) {
}

HealthDetail MqHealthIndicator::check() const {
    HealthDetail detail;
    detail.name = m_name;

    auto start = std::chrono::steady_clock::now();
    try {
        if (m_checkFunc()) {
            detail.status = HealthStatus::UP;
            detail.message = "MQ connection is healthy";
        } else {
            detail.status = HealthStatus::DOWN;
            detail.message = "MQ connection check failed";
        }
    } catch (const std::exception& e) {
        detail.status = HealthStatus::DOWN;
        detail.message = std::string("MQ check exception: ") + e.what();
    } catch (...) { // Blanket catch: capture unknown exception in MQ health check
        detail.status = HealthStatus::DOWN;
        detail.message = "MQ check unknown exception";
    }
    detail.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    return detail;
}

// ============================================================================
// NetworkHealthIndicator 实现
// ============================================================================

NetworkHealthIndicator::NetworkHealthIndicator(CheckFunc checkFunc, std::string name)
    : m_checkFunc(std::move(checkFunc)), m_name(std::move(name)) {
}

HealthDetail NetworkHealthIndicator::check() const {
    HealthDetail detail;
    detail.name = m_name;

    auto start = std::chrono::steady_clock::now();
    try {
        if (m_checkFunc()) {
            detail.status = HealthStatus::UP;
            detail.message = "Network connection is healthy";
        } else {
            detail.status = HealthStatus::DOWN;
            detail.message = "Network connection check failed";
        }
    } catch (const std::exception& e) {
        detail.status = HealthStatus::DOWN;
        detail.message = std::string("Network check exception: ") + e.what();
    } catch (...) {
        detail.status = HealthStatus::DOWN;
        detail.message = "Network check unknown exception";
    }
    detail.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    return detail;
}

// ============================================================================
// ResourcePoolHealthIndicator 实现
// ============================================================================

ResourcePoolHealthIndicator::ResourcePoolHealthIndicator(
    std::string poolName, std::function<double()> getUtilization, double threshold)
    : m_poolName(std::move(poolName))
    , m_getUtilization(std::move(getUtilization))
    , m_threshold(threshold) {
}

HealthDetail ResourcePoolHealthIndicator::check() const {
    HealthDetail detail;
    detail.name = m_poolName;

    auto start = std::chrono::steady_clock::now();
    try {
        double utilization = m_getUtilization();
        std::ostringstream msg;
        msg << "Utilization: " << (utilization * 100.0) << "%";

        if (utilization < m_threshold) {
            detail.status = HealthStatus::UP;
        } else {
            detail.status = HealthStatus::DOWN;
            msg << " (exceeds threshold " << (m_threshold * 100.0) << "%)";
        }
        detail.message = msg.str();
    } catch (const std::exception& e) {
        detail.status = HealthStatus::DOWN;
        detail.message = std::string("Resource pool check exception: ") + e.what();
    } catch (...) {
        detail.status = HealthStatus::DOWN;
        detail.message = "Resource pool check unknown exception";
    }
    detail.responseTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    return detail;
}

} // namespace server
} // namespace sc