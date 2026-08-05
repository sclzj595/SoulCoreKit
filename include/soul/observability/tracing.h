#ifndef SOUL_OBSERVABILITY_TRACING_H
#define SOUL_OBSERVABILITY_TRACING_H

// ============================================================================
// tracing.h — 分布式追踪 [v1.9.3 新增]
// ============================================================================
//
// 支持 W3C Trace Context 标准,用于跨服务调用链追踪。
//
// 核心概念:
//   - TraceId: 全局唯一标识一次分布式调用链(16 字节 hex)
//   - SpanId:  标识调用链中的一个操作(8 字节 hex)
//   - Traceparent: W3C 标准 HTTP header (version-traceId-spanId-flags)
//   - Tracestate: 供应商特定追踪数据
//
// 用法:
//   // 服务端: 从 HTTP 请求头提取 trace 上下文
//   auto span = Tracer::instance().startSpan("handle_request",
//       Tracer::extractFromHeaders(request.headers()));
//
//   // 客户端: 注入 trace 上下文到 HTTP 请求头
//   Tracer::injectToHeaders(span, httpRequest);
//
//   // 结束 span
//   span->end();
//
// W3C Trace Context 规范:
//   https://www.w3.org/TR/trace-context/

#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include <QString>
#include <QMap>

#include "soul/utils/json/json_helper.h"  // [v1.9.4] OtlpExporter 依赖 sc::json

namespace sc {
namespace observability {

// ============================================================================
// SpanContext — W3C Trace Context
// ============================================================================
struct SpanContext {
    std::string traceId;      ///< 16 字节 hex (32 字符)
    std::string spanId;       ///< 8 字节 hex (16 字符)
    std::string parentSpanId; ///< 父 span ID (根 span 为空)
    bool        sampled = true; ///< 是否采样

    /// @return 是否为有效的 trace context
    bool isValid() const { return !traceId.empty() && !spanId.empty(); }
};

// ============================================================================
// Span — 追踪跨度
// ============================================================================
//
// @thread_safety 线程安全 — 所有公开方法可并发调用。
//                写入方法(setTag/addEvent/setStatus/end)持互斥锁保护;
//                读取方法(isEnded/getTags/getEvents/isOk/statusDescription/durationMs)
//                持锁返回快照或原子读取。
class Span {
public:
    using Clock = std::chrono::steady_clock;

    /// @brief 跨度内的事件
    struct Event {
        std::string name;
        Clock::time_point timestamp;
        std::map<std::string, std::string> attributes;
    };

    Span(SpanContext context, std::string name)
        : m_context(std::move(context))
        , m_name(std::move(name))
        , m_startTime(Clock::now()) {}

    /// @brief 结束 span,记录耗时
    /// @thread_safety 线程安全
    void end() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_endTime = Clock::now();
        m_ended.store(true, std::memory_order_release);
    }

    /// @return span 是否已结束
    /// @thread_safety 线程安全(atomic acquire 读)
    bool isEnded() const {
        return m_ended.load(std::memory_order_acquire);
    }

    /// @brief 添加标签
    /// @thread_safety 线程安全
    void setTag(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_tags[key] = value;
    }

    /// @brief 添加事件
    /// @thread_safety 线程安全
    void addEvent(const std::string& name,
                  const std::map<std::string, std::string>& attributes = {}) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_events.push_back({name, Clock::now(), attributes});
    }

    /// @brief 设置状态
    /// @thread_safety 线程安全
    void setStatus(bool ok, const std::string& description = "") {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_ok = ok;
        m_statusDescription = description;
    }

    const SpanContext& context() const { return m_context; }
    const std::string& name() const { return m_name; }

    /// @return 标签映射快照(线程安全,返回副本)
    std::map<std::string, std::string> getTags() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_tags;
    }

    /// @return 事件列表快照(线程安全,返回副本)
    std::vector<Event> getEvents() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_events;
    }

    /// @return 是否成功(线程安全)
    bool isOk() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_ok;
    }

    /// @return 状态描述(线程安全,返回副本)
    std::string statusDescription() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_statusDescription;
    }

    /// @return 耗时(毫秒),未结束时返回 0
    /// @thread_safety 线程安全
    int64_t durationMs() const {
        if (!m_ended.load(std::memory_order_acquire)) return 0;
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            m_endTime - m_startTime).count();
    }

    /// @return 开始时间(纳秒),用于 OtlpExporter 时间戳 [v1.9.4]
    /// @thread_safety 线程安全
    int64_t startTimeNs() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            m_startTime.time_since_epoch()).count();
    }

    /// @return 结束时间(纳秒),未结束时返回 0 [v1.9.4]
    /// @thread_safety 线程安全
    int64_t endTimeNs() const {
        if (!m_ended.load(std::memory_order_acquire)) return 0;
        std::lock_guard<std::mutex> lock(m_mutex);
        return std::chrono::duration_cast<std::chrono::nanoseconds>(
            m_endTime.time_since_epoch()).count();
    }

private:
    SpanContext m_context;
    std::string m_name;
    Clock::time_point m_startTime;
    Clock::time_point m_endTime;
    std::map<std::string, std::string> m_tags;
    bool m_ok = true;
    std::atomic<bool> m_ended{false};
    std::string m_statusDescription;
    std::vector<Event> m_events;
    mutable std::mutex m_mutex;
};

// ============================================================================
// SpanGuard — RAII Span 管理 [v1.9.3]
// ============================================================================
//
// 构造时持有 Span,析构时自动调用 end()。
// 确保 span 在异常路径上也能正确结束并记录耗时。
//
// 用法:
//   SpanGuard guard(Tracer::instance().startSpan("handleRequest"));
//   guard->setTag("handler", "UserController");
//   // ... 业务逻辑,抛异常也会自动 end() ...
//
// @thread_safety 非线程安全 — 同一 SpanGuard 实例不应在多线程间共享
class SpanGuard {
public:
    explicit SpanGuard(std::shared_ptr<Span> span) : m_span(std::move(span)) {}
    ~SpanGuard() {
        if (m_span && !m_span->isEnded()) {
            m_span->end();
        }
    }

    SpanGuard(const SpanGuard&) = delete;
    SpanGuard& operator=(const SpanGuard&) = delete;
    SpanGuard(SpanGuard&&) = default;
    SpanGuard& operator=(SpanGuard&&) = default;

    Span* operator->() const { return m_span.get(); }
    Span& operator*() const { return *m_span; }
    std::shared_ptr<Span> span() const { return m_span; }

private:
    std::shared_ptr<Span> m_span;
};

// ============================================================================
// Tracer — 分布式追踪器
// ============================================================================
class Tracer {
public:
    static Tracer& instance();

    // ========================================================================
    // 启用/禁用控制
    // ========================================================================

    /// @brief 设置是否启用追踪(禁用时 startSpan 返回 nullptr)
    void setEnabled(bool enabled) { m_enabled.store(enabled, std::memory_order_release); }

    /// @return 是否启用追踪
    bool isEnabled() const { return m_enabled.load(std::memory_order_acquire); }

    // ========================================================================
    // Span 管理
    // ========================================================================

    /// @brief 创建新的根 span
    /// @return 追踪禁用时返回 nullptr
    std::shared_ptr<Span> startSpan(const std::string& name);

    /// @brief 从父 context 创建子 span
    /// @return 追踪禁用时返回 nullptr
    std::shared_ptr<Span> startSpan(const std::string& name, const SpanContext& parent);

    // ========================================================================
    // W3C Trace Context 提取/注入
    // ========================================================================

    /// @brief 从 HTTP 请求头提取 trace context
    /// @param headers HTTP 请求头(key 已转小写)
    static SpanContext extractFromHeaders(const QMap<QString, QString>& headers);

    /// @brief 将 trace context 注入到 HTTP 请求头
    /// @param span    当前 span
    /// @param headers 输出: HTTP 请求头
    static void injectToHeaders(const std::shared_ptr<Span>& span,
                                 QMap<QString, QString>& headers);

    /// @brief 解析 traceparent header
    /// @param header "00-traceId-spanId-flags" 格式
    static SpanContext parseTraceparent(const std::string& header);

    /// @brief 生成 traceparent header
    static std::string formatTraceparent(const SpanContext& context);

    // ========================================================================
    // ID 生成
    // ========================================================================

    /// @brief 生成 16 字节 hex trace ID
    static std::string generateTraceId();

    /// @brief 生成 8 字节 hex span ID
    static std::string generateSpanId();

private:
    Tracer();
    std::atomic<bool> m_enabled{true};  ///< v1.9.3: 追踪启停开关
};

// ============================================================================
// OtlpExporter — OpenTelemetry OTLP 导出器 [v1.9.4]
// ============================================================================
//
// 将已结束的 Span 序列化为 OTLP JSON 格式,支持发送到 OpenTelemetry Collector。
//
// OTLP/HTTP JSON 规范:
//   POST /v1/traces
//   Content-Type: application/json
//   Body: {"resourceSpans": [{"scopeSpans": [{"spans": [...]}]}]}
//
// 用法:
//   OtlpExporter exporter("http://localhost:4318/v1/traces");
//   exporter.serialize(spans);
//
// @thread_safety 线程安全 — 内部 mutex 保护状态
class OtlpExporter {
public:
    /// @brief 构造导出器
    /// @param endpoint OTLP Collector HTTP 端点 (如 "http://localhost:4318/v1/traces")
    explicit OtlpExporter(std::string endpoint = "")
        : m_endpoint(std::move(endpoint)) {}

    /// @brief 设置 Collector 端点
    void setEndpoint(const std::string& endpoint) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_endpoint = endpoint;
    }

    /// @brief 获取当前端点
    std::string endpoint() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_endpoint;
    }

    /// @brief 将 Span 列表序列化为 OTLP JSON
    /// @param spans 已结束的 Span 列表
    /// @return OTLP JSON 格式的 QByteArray
    static QByteArray serializeSpans(const std::vector<std::shared_ptr<Span>>& spans) {
        sc::json::Json root = sc::json::Json::object();
        sc::json::Json resourceSpans = sc::json::Json::array();
        sc::json::Json resourceSpan = sc::json::Json::object();

        // resource
        sc::json::Json resource = sc::json::Json::object();
        sc::json::Json resourceAttrs = sc::json::Json::array();
        sc::json::Json serviceNameAttr = sc::json::Json::object();
        serviceNameAttr["key"] = "service.name";
        sc::json::Json serviceNameValue = sc::json::Json::object();
        serviceNameValue["stringValue"] = "SoulCoreKit";
        serviceNameAttr["value"] = serviceNameValue;
        resourceAttrs.push_back(serviceNameAttr);
        resource["attributes"] = resourceAttrs;
        resourceSpan["resource"] = resource;

        // scopeSpans
        sc::json::Json scopeSpans = sc::json::Json::array();
        sc::json::Json scopeSpan = sc::json::Json::object();

        sc::json::Json scope = sc::json::Json::object();
        scope["name"] = "sc.observability";
        scopeSpan["scope"] = scope;

        sc::json::Json spanArray = sc::json::Json::array();
        for (const auto& span : spans) {
            if (!span || !span->isEnded()) continue;
            spanArray.push_back(spanToJson(span));
        }
        scopeSpan["spans"] = spanArray;
        scopeSpans.push_back(scopeSpan);
        resourceSpan["scopeSpans"] = scopeSpans;
        resourceSpans.push_back(resourceSpan);
        root["resourceSpans"] = resourceSpans;

        return sc::json::serialize(root);
    }

    /// @brief 将 Span 列表序列化为 OTLP JSON (实例方法,委托静态 serializeSpans)
    /// @param spans 已结束的 Span 列表
    /// @return OTLP JSON 格式的 QByteArray
    QByteArray serialize(const std::vector<std::shared_ptr<Span>>& spans) {
        return serializeSpans(spans);
    }

private:
    /// @brief 将单个 Span 转为 OTLP JSON
    static sc::json::Json spanToJson(const std::shared_ptr<Span>& span) {
        sc::json::Json j = sc::json::Json::object();

        const auto& ctx = span->context();
        j["traceId"] = ctx.traceId;
        j["spanId"] = ctx.spanId;
        if (!ctx.parentSpanId.empty()) {
            j["parentSpanId"] = ctx.parentSpanId;
        }
        j["name"] = span->name();

        // 时间戳 (Unix nanoseconds) [v1.9.4] 使用 Span 实际起止时间
        j["startTimeUnixNano"] = std::to_string(span->startTimeNs());
        j["endTimeUnixNano"] = std::to_string(span->endTimeNs());

        // 状态
        sc::json::Json status = sc::json::Json::object();
        status["code"] = span->isOk() ? 1 : 2;  // 1=OK, 2=ERROR
        if (!span->statusDescription().empty()) {
            status["message"] = span->statusDescription();
        }
        j["status"] = status;

        // 标签 → attributes
        sc::json::Json attrs = sc::json::Json::array();
        for (const auto& [key, value] : span->getTags()) {
            sc::json::Json attr = sc::json::Json::object();
            attr["key"] = key;
            sc::json::Json val = sc::json::Json::object();
            val["stringValue"] = value;
            attr["value"] = val;
            attrs.push_back(attr);
        }
        j["attributes"] = attrs;

        // 事件
        sc::json::Json events = sc::json::Json::array();
        for (const auto& event : span->getEvents()) {
            sc::json::Json ev = sc::json::Json::object();
            ev["name"] = event.name;
            ev["timeUnixNano"] = std::to_string(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    event.timestamp.time_since_epoch()).count());
            sc::json::Json evAttrs = sc::json::Json::array();
            for (const auto& [k, v] : event.attributes) {
                sc::json::Json attr = sc::json::Json::object();
                attr["key"] = k;
                sc::json::Json val = sc::json::Json::object();
                val["stringValue"] = v;
                attr["value"] = val;
                evAttrs.push_back(attr);
            }
            ev["attributes"] = evAttrs;
            events.push_back(ev);
        }
        if (!events.empty()) {
            j["events"] = events;
        }

        return j;
    }

    mutable std::mutex m_mutex;
    std::string m_endpoint;
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_TRACING_H
