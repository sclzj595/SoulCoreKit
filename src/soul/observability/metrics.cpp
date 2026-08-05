#include "soul/observability/metrics.h"

#include <algorithm>
#include <cmath>

namespace sc {
namespace observability {

// ============================================================================
// HistogramSnapshot
// ============================================================================

double HistogramSnapshot::quantile(double q) const noexcept {
    if (q < 0.0 || q > 1.0) return 0.0;
    if (count == 0) return 0.0;

    // 目标秩：第 q * count 个观测值
    double target = q * static_cast<double>(count);
    std::uint64_t cumulative = 0;

    // 遍历桶，线性插值估算分位数
    for (std::size_t i = 0; i < bucketCounts.size(); ++i) {
        cumulative += bucketCounts[i];
        if (static_cast<double>(cumulative) >= target) {
            // 在第 i 个桶内，线性插值
            double bucketStart = (i == 0) ? min : boundaries[i - 1];
            double bucketEnd   = (i < boundaries.size()) ? boundaries[i] : max;

            std::uint64_t prevCumulative = cumulative - bucketCounts[i];
            double bucketFraction = static_cast<double>(bucketCounts[i]);
            if (bucketFraction <= 0.0) {
                return bucketEnd;
            }

            double fraction = (target - static_cast<double>(prevCumulative)) / bucketFraction;
            return bucketStart + fraction * (bucketEnd - bucketStart);
        }
    }
    return max;
}

// ============================================================================
// Counter
// ============================================================================

void Counter::increment(const Labels& labels, double delta) {
    if (delta < 0) return;  // Counter 不允许递减
    std::lock_guard<std::mutex> lock(m_mutex);
    m_labeledValues[labels] += delta;
}

std::map<Labels, double> Counter::labeledValues() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_labeledValues;
}

// ============================================================================
// Histogram
// ============================================================================

std::size_t Histogram::findBucket(double value) const noexcept {
    // 在 boundaries 中找到第一个 >= value 的位置
    auto it = std::lower_bound(m_buckets.boundaries.begin(),
                                m_buckets.boundaries.end(),
                                value);
    return static_cast<std::size_t>(it - m_buckets.boundaries.begin());
}

void Histogram::observe(double value) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 延迟初始化桶计数
    if (m_bucketCounts.empty()) {
        m_bucketCounts.resize(m_buckets.bucketCount(), 0);
    }

    // 更新统计量
    if (m_count == 0) {
        m_min = value;
        m_max = value;
    } else {
        m_min = std::min(m_min, value);
        m_max = std::max(m_max, value);
    }
    m_sum += value;
    m_count += 1;

    // 更新桶计数
    std::size_t idx = findBucket(value);
    m_bucketCounts[idx] += 1;
}

HistogramSnapshot Histogram::snapshot() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    HistogramSnapshot snap;
    snap.min        = m_min;
    snap.max        = m_max;
    snap.sum        = m_sum;
    snap.count      = m_count;
    snap.bucketCounts = m_bucketCounts;
    snap.boundaries  = m_buckets.boundaries;

    // 确保快照中 bucketCounts 不为空
    if (snap.bucketCounts.empty()) {
        snap.bucketCounts.resize(m_buckets.bucketCount(), 0);
    }

    return snap;
}

// ============================================================================
// MetricsRegistry
// ============================================================================

MetricsRegistry& MetricsRegistry::instance() {
    static MetricsRegistry inst;
    return inst;
}

Counter& MetricsRegistry::counter(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_counters.find(name);
    if (it != m_counters.end()) {
        return *it->second;
    }
    auto ptr = std::make_shared<Counter>(name, help);
    auto& ref = *ptr;
    m_counters[name] = std::move(ptr);
    return ref;
}

Gauge& MetricsRegistry::gauge(const std::string& name, const std::string& help) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_gauges.find(name);
    if (it != m_gauges.end()) {
        return *it->second;
    }
    auto ptr = std::make_shared<Gauge>(name, help);
    auto& ref = *ptr;
    m_gauges[name] = std::move(ptr);
    return ref;
}

Histogram& MetricsRegistry::histogram(const std::string& name,
                                       const std::string& help,
                                       const HistogramBuckets& buckets) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_histograms.find(name);
    if (it != m_histograms.end()) {
        return *it->second;
    }
    auto ptr = std::make_shared<Histogram>(name, help, buckets);
    auto& ref = *ptr;
    m_histograms[name] = std::move(ptr);
    return ref;
}

std::vector<std::shared_ptr<IMetric>> MetricsRegistry::allMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<IMetric>> result;
    result.reserve(m_counters.size() + m_gauges.size() + m_histograms.size());
    for (const auto& [_, p] : m_counters) {
        result.push_back(p);
    }
    for (const auto& [_, p] : m_gauges) {
        result.push_back(p);
    }
    for (const auto& [_, p] : m_histograms) {
        result.push_back(p);
    }
    return result;
}

void MetricsRegistry::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_counters.clear();
    m_gauges.clear();
    m_histograms.clear();
}

} // namespace observability
} // namespace sc
