#ifndef SOUL_OBSERVABILITY_PROMETHEUS_EXPORTER_H
#define SOUL_OBSERVABILITY_PROMETHEUS_EXPORTER_H

// ============================================================================
// prometheus_exporter.h — Prometheus 指标导出器 [v1.9.3 新增]
// ============================================================================
//
// 将 MetricsRegistry 中的 Counter/Gauge/Histogram 导出为 Prometheus
// 文本格式(OpenMetrics exposition format)。
//
// 用法:
//   // 注册到 HttpServer
//   server.get("/metrics", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "text/plain; version=0.0.4");
//       resp.setBody(PrometheusExporter::exportMetrics());
//   });
//
// 输出示例:
//   # HELP http_requests_total Total HTTP requests
//   # TYPE http_requests_total counter
//   http_requests_total{method="GET",status="200"} 42
//   http_requests_total{method="POST",status="201"} 15
//   # HELP memory_usage_bytes Current memory usage
//   # TYPE memory_usage_bytes gauge
//   memory_usage_bytes 1048576
//   # HELP http_request_duration_ms HTTP request duration
//   # TYPE http_request_duration_ms histogram
//   http_request_duration_ms_bucket{le="1"} 0
//   http_request_duration_ms_bucket{le="5"} 3
//   http_request_duration_ms_bucket{le="+Inf"} 10
//   http_request_duration_ms_sum 42.5
//   http_request_duration_ms_count 10

#include "soul/observability/metrics.h"

#include <QByteArray>
#include <sstream>

namespace sc {
namespace observability {

class PrometheusExporter {
public:
    /// @brief 导出所有已注册指标为 Prometheus 文本格式
    static QByteArray exportMetrics() {
        auto metrics = MetricsRegistry::instance().allMetrics();
        std::ostringstream oss;

        for (const auto* metric : metrics) {
            switch (metric->type()) {
            case MetricType::Counter:
                exportCounter(oss, static_cast<const Counter*>(metric));
                break;
            case MetricType::Gauge:
                exportGauge(oss, static_cast<const Gauge*>(metric));
                break;
            case MetricType::Histogram:
                exportHistogram(oss, static_cast<const Histogram*>(metric));
                break;
            }
        }

        return QByteArray::fromStdString(oss.str());
    }

private:
    static std::string escapeLabelValue(const std::string& value) {
        std::string result;
        result.reserve(value.size());
        for (char c : value) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '"':  result += "\\\""; break;
                case '\n': result += "\\n"; break;
                default:   result += c; break;
            }
        }
        return result;
    }

    static std::string escapeHelpText(const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (char c : text) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                default:   result += c; break;
            }
        }
        return result;
    }

    static std::string formatLabels(const Labels& labels) {
        if (labels.empty()) return "";
        std::ostringstream oss;
        oss << "{";
        bool first = true;
        for (const auto& [key, value] : labels) {
            if (!first) oss << ",";
            oss << key.toStdString() << "=\"" << escapeLabelValue(value.toStdString()) << "\"";
            first = false;
        }
        oss << "}";
        return oss.str();
    }

    static void exportCounter(std::ostringstream& oss, const Counter* counter) {
        // HELP / TYPE
        oss << "# HELP " << counter->name() << " " << escapeHelpText(counter->help()) << "\n";
        oss << "# TYPE " << counter->name() << " counter\n";

        // 无标签值
        double val = counter->value();
        if (val > 0.0) {
            oss << counter->name() << " " << val << "\n";
        }

        // 带标签值
        for (const auto& [labels, labeledVal] : counter->labeledValues()) {
            if (labeledVal > 0.0) {
                oss << counter->name() << formatLabels(labels) << " " << labeledVal << "\n";
            }
        }
        oss << "\n";
    }

    static void exportGauge(std::ostringstream& oss, const Gauge* gauge) {
        oss << "# HELP " << gauge->name() << " " << escapeHelpText(gauge->help()) << "\n";
        oss << "# TYPE " << gauge->name() << " gauge\n";

        // 无标签值
        oss << gauge->name() << " " << gauge->value() << "\n";

        // 带标签值
        for (const auto& [labels, labeledVal] : gauge->labeledValues()) {
            oss << gauge->name() << formatLabels(labels) << " " << labeledVal << "\n";
        }
        oss << "\n";
    }

    static void exportHistogram(std::ostringstream& oss, const Histogram* histogram) {
        auto snap = histogram->snapshot();
        const auto& buckets = histogram->buckets();

        oss << "# HELP " << histogram->name() << " " << escapeHelpText(histogram->help()) << "\n";
        oss << "# TYPE " << histogram->name() << " histogram\n";

        // 桶计数
        std::uint64_t cumulative = 0;
        for (std::size_t i = 0; i < snap.bucketCounts.size(); ++i) {
            cumulative += snap.bucketCounts[i];

            std::string le;
            if (i < snap.boundaries.size()) {
                le = std::to_string(snap.boundaries[i]);
            } else {
                le = "+Inf";
            }

            oss << histogram->name() << "_bucket{le=\"" << le << "\"} "
                << cumulative << "\n";
        }

        // sum / count
        oss << histogram->name() << "_sum " << snap.sum << "\n";
        oss << histogram->name() << "_count " << snap.count << "\n";
        oss << "\n";
    }
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_PROMETHEUS_EXPORTER_H