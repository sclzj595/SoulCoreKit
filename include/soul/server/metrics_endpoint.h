#ifndef SOUL_SERVER_METRICS_ENDPOINT_H
#define SOUL_SERVER_METRICS_ENDPOINT_H

// ============================================================================
// metrics_endpoint.h — 指标端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/metrics,暴露 MetricsRegistry 中注册的
// 运行时指标 (Counter/Gauge/Histogram)。
//
// 用法:
//   server.get("/actuator/metrics", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(MetricsEndpoint::toJson());
//   });
//
//   server.get("/actuator/metrics/{name}",
//       [](const HttpRequest& req, HttpResponse& resp) {
//           resp.setHeader("Content-Type", "application/json");
//           resp.setBody(MetricsEndpoint::getMetric(req.pathParam("name")));
//       });

#include "soul/observability/metrics.h"
#include "soul/utils/json/json_helper.h"

#include <QByteArray>
#include <memory>
#include <string>

namespace sc {
namespace server {

// ============================================================================
// MetricsEndpoint — 指标端点
// ============================================================================
//
// 依赖 sc::observability::MetricsRegistry 单例,所有方法均为静态调用。
//
// @thread_safety 依赖 MetricsRegistry 的线程安全保证
class MetricsEndpoint {
public:
    /// @brief 列出所有已注册指标名
    /// @return JSON: {"names": ["requests_total", "cpu_usage", ...]}
    static QByteArray listMetricNames() {
        auto metrics = sc::observability::MetricsRegistry::instance().allMetrics();
        sc::json::Json root  = sc::json::Json::object();
        sc::json::Json names = sc::json::Json::array();
        for (const auto& metric : metrics) {
            names.push_back(metric->name());
        }
        root["names"] = names;
        return sc::json::serializePretty(root);
    }

    /// @brief 获取单个指标详情
    /// @param name 指标名称
    /// @return 找到时返回 {"name":..., "type":"counter/gauge/histogram", ...}
    ///         找不到时返回 {"error": "metric not found"}
    static QByteArray getMetric(const std::string& name) {
        auto metrics = sc::observability::MetricsRegistry::instance().allMetrics();
        for (const auto& metric : metrics) {
            if (metric->name() != name) {
                continue;
            }

            sc::json::Json root = sc::json::Json::object();
            root["name"] = metric->name();

            switch (metric->type()) {
                case sc::observability::MetricType::Counter: {
                    auto counter = std::dynamic_pointer_cast<
                        sc::observability::Counter>(metric);
                    root["type"]  = "counter";
                    root["value"] = counter ? counter->value() : 0.0;
                    break;
                }
                case sc::observability::MetricType::Gauge: {
                    auto gauge = std::dynamic_pointer_cast<
                        sc::observability::Gauge>(metric);
                    root["type"]  = "gauge";
                    root["value"] = gauge ? gauge->value() : 0.0;
                    break;
                }
                case sc::observability::MetricType::Histogram: {
                    auto hist = std::dynamic_pointer_cast<
                        sc::observability::Histogram>(metric);
                    root["type"] = "histogram";
                    sc::json::Json snapshot = sc::json::Json::object();
                    if (hist) {
                        auto snap = hist->snapshot();
                        snapshot["count"] = snap.count;
                        snapshot["sum"]   = snap.sum;
                        snapshot["min"]   = snap.min;
                        snapshot["max"]   = snap.max;
                    }
                    root["snapshot"] = snapshot;
                    break;
                }
            }
            return sc::json::serializePretty(root);
        }

        // 找不到指定名称的指标
        sc::json::Json err = sc::json::Json::object();
        err["error"] = "metric not found";
        return sc::json::serializePretty(err);
    }

    /// @brief 默认入口,等同于 listMetricNames()
    static QByteArray toJson() { return listMetricNames(); }
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_METRICS_ENDPOINT_H
