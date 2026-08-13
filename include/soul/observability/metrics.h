#ifndef SOUL_OBSERVABILITY_METRICS_H
#define SOUL_OBSERVABILITY_METRICS_H

#include <QString>
#include <QVariant>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace sc {
namespace observability {

/**
 * @brief 指标类型枚举
 */
enum class MetricType {
    Counter,    ///< 计数器（单调递增）
    Gauge,      ///< 仪表（可增可减）
    Histogram   ///< 直方图（分桶统计）
};

/**
 * @brief 指标标签（键值对，用于多维度聚合）
 */
using Labels = std::map<QString, QString>;

/**
 * @brief 直方图桶配置
 *
 * 定义直方图的分桶边界，例如 [10, 50, 100, 500, 1000] 表示：
 * - bucket_0: (-inf, 10]
 * - bucket_1: (10, 50]
 * - bucket_2: (50, 100]
 * - ...
 * - bucket_inf: (1000, +inf)
 */
struct HistogramBuckets {
    std::vector<double> boundaries;  ///< 升序排列的桶上界

    /**
     * @brief 创建默认的延迟分桶（适合毫秒级延迟统计）
     * 桶: 1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000 ms
     */
    static HistogramBuckets defaultLatency() {
        return {{1, 5, 10, 25, 50, 100, 250, 500, 1000, 2500, 5000, 10000}};
    }

    /**
     * @brief 创建默认的请求大小分桶
     * 桶: 64, 128, 256, 512, 1024, 4096, 16384, 65536, 262144, 1048576 字节
     */
    static HistogramBuckets defaultSize() {
        return {{64, 128, 256, 512, 1024, 4096, 16384, 65536, 262144, 1048576}};
    }

    /**
     * @brief 获取桶数量（含 +inf 桶）
     */
    [[nodiscard]] std::size_t bucketCount() const noexcept {
        return boundaries.size() + 1;
    }
};

/**
 * @brief 直方图快照（用于导出统计数据）
 */
struct HistogramSnapshot {
    double                min         = 0.0;
    double                max         = 0.0;
    double                sum         = 0.0;
    std::uint64_t         count       = 0;
    std::vector<std::uint64_t> bucketCounts;  ///< 各桶计数（含 +inf 桶）
    std::vector<double>         boundaries;   ///< 桶边界（与 bucketCounts 对齐，最后一项为 +inf）

    /**
     * @brief 计算平均值
     */
    [[nodiscard]] double mean() const noexcept {
        return count > 0 ? sum / static_cast<double>(count) : 0.0;
    }

    /**
     * @brief 估算分位数（线性插值）
     * @param q 分位数 [0.0, 1.0]
     * @return 估算值
     */
    [[nodiscard]] double quantile(double q) const noexcept;
};

/**
 * @class IMetric
 * @brief 指标接口
 *
 * 所有指标类型（Counter/Gauge/Histogram）的抽象基类。
 * 通过 MetricsRegistry 注册和查找。
 *
 * @thread_safety Implementations are thread-safe.
 */
class IMetric {
public:
    virtual ~IMetric() = default;

    /// @return 指标名称
    [[nodiscard]] virtual const std::string& name() const = 0;

    /// @return 指标描述
    [[nodiscard]] virtual const std::string& help() const = 0;

    /// @return 指标类型
    [[nodiscard]] virtual MetricType type() const = 0;
};

/**
 * @class Counter
 * @brief 计数器指标（单调递增）
 *
 * 适用于请求计数、错误计数、字节数等只增不减的场景。
 *
 * @par 使用示例
 * @code
 * auto& counter = MetricsRegistry::instance().counter("http_requests_total",
 *                                                      "Total HTTP requests");
 * counter.increment();
 * counter.increment(5);
 * counter.increment({{"method", "GET"}, {"status", "200"}});
 * @endcode
 *
 * @thread_safety Thread-Safe — 使用 mutex 保护并发访问
 */
class Counter : public IMetric {
public:
    Counter(std::string name, std::string help)
        : m_name(std::move(name))
        , m_help(std::move(help)) {}

    /// @brief 无标签自增 1
    void increment() { increment(1.0); }

    /// @brief 无标签自增指定值（必须 >= 0，负值将被忽略）
    void increment(double delta) {
        if (delta < 0) return;  // Counter 不允许递减
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value += delta;
    }

    /// @brief 带标签自增 1
    void increment(const Labels& labels) { increment(labels, 1.0); }

    /// @brief 带标签自增指定值
    void increment(const Labels& labels, double delta);

    /// @brief 获取无标签当前值
    [[nodiscard]] double value() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_value;
    }

    /// @brief 获取所有标签组合的值
    [[nodiscard]] std::map<Labels, double> labeledValues() const;

    const std::string& name() const override { return m_name; }
    const std::string& help() const override { return m_help; }
    MetricType type() const override { return MetricType::Counter; }

private:
    std::string m_name;
    std::string m_help;
    double      m_value = 0.0;
    mutable std::mutex m_mutex;
    std::map<Labels, double> m_labeledValues;  ///< 受 m_mutex 保护
};

/**
 * @class Gauge
 * @brief 仪表指标（可增可减）
 *
 * 适用于温度、内存使用、连接数等可上下波动的场景。
 *
 * @par 使用示例
 * @code
 * auto& gauge = MetricsRegistry::instance().gauge("memory_usage_bytes",
 *                                                  "Current memory usage");
 * gauge.set(1024 * 1024);
 * gauge.increment(512);
 * gauge.decrement(256);
 * @endcode
 *
 * @thread_safety Thread-Safe
 */
class Gauge : public IMetric {
public:
    Gauge(std::string name, std::string help)
        : m_name(std::move(name))
        , m_help(std::move(help)) {}

    void set(double value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value = value;
    }

    /// @brief 带标签设置值(v1.9.0 新增,与 Counter::increment(labels, delta) 对称)
    void set(const Labels& labels, double value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_labeledValues[labels] = value;
    }

    void increment(double delta = 1.0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value += delta;
    }

    void decrement(double delta = 1.0) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_value -= delta;
    }

    [[nodiscard]] double value() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_value;
    }

    /// @brief 获取所有标签组合的值(v1.9.0 新增)
    [[nodiscard]] std::map<Labels, double> labeledValues() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_labeledValues;
    }

    const std::string& name() const override { return m_name; }
    const std::string& help() const override { return m_help; }
    MetricType type() const override { return MetricType::Gauge; }

private:
    std::string       m_name;
    std::string       m_help;
    double            m_value = 0.0;
    mutable std::mutex m_mutex;
    std::map<Labels, double> m_labeledValues;  ///< 受 m_mutex 保护(v1.9.0 新增)
};

/**
 * @class Histogram
 * @brief 直方图指标（分桶统计）
 *
 * 适用于延迟分布、响应大小分布等需要统计分布的场景。
 * 自动维护 min/max/sum/count 和分桶计数。
 *
 * @par 使用示例
 * @code
 * auto& hist = MetricsRegistry::instance().histogram(
 *     "http_request_duration_ms",
 *     "HTTP request duration in milliseconds",
 *     HistogramBuckets::defaultLatency());
 *
 * hist.observe(42.5);  // 观察到一个 42.5ms 的请求
 * auto snapshot = hist.snapshot();
 * qDebug() << "p99:" << snapshot.quantile(0.99);
 * @endcode
 *
 * @thread_safety Thread-Safe
 */
class Histogram : public IMetric {
public:
    Histogram(std::string name, std::string help, HistogramBuckets buckets)
        : m_name(std::move(name))
        , m_help(std::move(help))
        , m_buckets(std::move(buckets)) {
        // 校验:boundaries 不能为空,且必须升序排列
        if (m_buckets.boundaries.empty()) {
            throw std::invalid_argument("Histogram: buckets.boundaries must not be empty");
        }
        for (std::size_t i = 1; i < m_buckets.boundaries.size(); ++i) {
            if (m_buckets.boundaries[i] <= m_buckets.boundaries[i - 1]) {
                throw std::invalid_argument("Histogram: buckets.boundaries must be strictly ascending");
            }
        }
        // [v1.9.2] 构造时预初始化桶计数,避免首次 observe() 时延迟分配影响性能
        m_bucketCounts.resize(m_buckets.bucketCount(), 0);
    }

    /// @brief 观察一个值
    void observe(double value);

    /// @brief 获取当前快照
    [[nodiscard]] HistogramSnapshot snapshot() const;

    /// @brief 获取桶配置
    [[nodiscard]] const HistogramBuckets& buckets() const noexcept { return m_buckets; }

    const std::string& name() const override { return m_name; }
    const std::string& help() const override { return m_help; }
    MetricType type() const override { return MetricType::Histogram; }

private:
    std::string         m_name;
    std::string         m_help;
    HistogramBuckets    m_buckets;
    mutable std::mutex  m_mutex;
    double              m_min   = 0.0;
    double              m_max   = 0.0;
    double              m_sum   = 0.0;
    std::uint64_t       m_count = 0;
    std::vector<std::uint64_t> m_bucketCounts;  ///< 延迟初始化（避免 mutex 内分配）

    /// @brief 确定 value 落入哪个桶
    [[nodiscard]] std::size_t findBucket(double value) const noexcept;
};

// ============================================================================
// Timer — 便捷计时器 [v2.8.0 新增]
// ============================================================================
// Timer 是 Histogram 的语义化封装，专用于延迟/耗时测量。
// 内部使用 Histogram + 默认延迟分桶，提供 RAII 计时和手动记录两种模式。
//
// 用法:
//   // 手动记录
//   Timer timer("http_request_duration", "HTTP request latency");
//   timer.record(42.5);  // 记录 42.5ms
//
//   // RAII 计时
//   {
//       Timer::Scoped scoped(timer);  // 开始计时
//       doWork();
//   }  // 析构时自动 record()

class Timer : public IMetric {
public:
    /// @brief RAII 计时作用域
    class Scoped {
    public:
        explicit Scoped(Timer& timer)
            : m_timer(timer), m_start(std::chrono::steady_clock::now()) {}
        ~Scoped() {
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - m_start);
            m_timer.record(static_cast<double>(elapsed.count()));
        }
        Scoped(const Scoped&) = delete;
        Scoped& operator=(const Scoped&) = delete;
    private:
        Timer& m_timer;
        std::chrono::steady_clock::time_point m_start;
    };

    Timer(std::string name, std::string help,
          HistogramBuckets buckets = HistogramBuckets::defaultLatency())
        : m_name(std::move(name))
        , m_help(std::move(help))
        , m_histogram(m_name + "_histogram", m_help, std::move(buckets)) {}

    /// @brief 记录耗时 (毫秒)
    void record(double latencyMs) { m_histogram.observe(latencyMs); }

    /// @brief 获取快照
    [[nodiscard]] HistogramSnapshot snapshot() const { return m_histogram.snapshot(); }

    const std::string& name() const override { return m_name; }
    const std::string& help() const override { return m_help; }
    MetricType type() const override { return MetricType::Histogram; }

private:
    std::string m_name;
    std::string m_help;
    Histogram m_histogram;
};

/**
 * @class MetricsRegistry
 * @brief 指标注册表（单例）
 *
 * 全局指标注册中心，统一管理所有 Counter/Gauge/Histogram 实例。
 * 指标按名称唯一注册，重复注册返回已存在的实例。
 *
 * @par 使用示例
 * @code
 * auto& counter = MetricsRegistry::instance().counter(
 *     "requests_total", "Total requests");
 * counter.increment();
 *
 * // 导出所有指标（用于 Prometheus 等）
 * auto metrics = MetricsRegistry::instance().allMetrics();
 * @endcode
 *
 * @thread_safety Thread-Safe
 */
class MetricsRegistry {
public:
    /// @brief 获取单例
    static MetricsRegistry& instance();

    /// @brief 获取或创建 Counter
    Counter& counter(const std::string& name, const std::string& help);

    /// @brief 获取或创建 Gauge
    Gauge& gauge(const std::string& name, const std::string& help);

    /// @brief 获取或创建 Histogram
    Histogram& histogram(const std::string& name,
                          const std::string& help,
                          const HistogramBuckets& buckets);

    /// @brief 获取所有已注册指标(shared_ptr 快照,线程安全) [v1.9.4]
    /// @return shared_ptr 副本,调用方持有期间指标不会被注销销毁
    [[nodiscard]] std::vector<std::shared_ptr<IMetric>> allMetrics() const;

    /// @brief 清空所有指标（仅用于测试）
    void clear();

private:
    MetricsRegistry() = default;
    MetricsRegistry(const MetricsRegistry&) = delete;
    MetricsRegistry& operator=(const MetricsRegistry&) = delete;

    mutable std::mutex m_mutex;
    std::map<std::string, std::shared_ptr<Counter>>   m_counters;
    std::map<std::string, std::shared_ptr<Gauge>>     m_gauges;
    std::map<std::string, std::shared_ptr<Histogram>> m_histograms;
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_METRICS_H
