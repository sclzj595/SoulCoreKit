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

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include <QString>
#include <QMap>

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
    void end() {
        m_endTime = Clock::now();
    }

    /// @brief 添加标签
    void setTag(const std::string& key, const std::string& value) {
        m_tags[key] = value;
    }

    /// @brief 添加事件
    void addEvent(const std::string& name, const std::map<std::string, std::string>& attributes = {}) {
        m_events.push_back({name, Clock::now(), attributes});
    }

    /// @brief 设置状态
    void setStatus(bool ok, const std::string& description = "") {
        m_ok = ok;
        m_statusDescription = description;
    }

    const SpanContext& context() const { return m_context; }
    const std::string& name() const { return m_name; }

    /// @return 标签映射(只读)
    const std::map<std::string, std::string>& getTags() const { return m_tags; }

    /// @return 事件列表(只读)
    const std::vector<Event>& getEvents() const { return m_events; }

    /// @return 是否成功
    bool isOk() const { return m_ok; }

    /// @return 状态描述
    const std::string& statusDescription() const { return m_statusDescription; }

    /// @return 耗时(毫秒)
    int64_t durationMs() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            m_endTime - m_startTime).count();
    }

private:
    SpanContext m_context;
    std::string m_name;
    Clock::time_point m_startTime;
    Clock::time_point m_endTime;
    std::map<std::string, std::string> m_tags;
    bool m_ok = true;
    std::string m_statusDescription;
    std::vector<Event> m_events;
};

// ============================================================================
// Tracer — 分布式追踪器
// ============================================================================
class Tracer {
public:
    static Tracer& instance();

    // ========================================================================
    // Span 管理
    // ========================================================================

    /// @brief 创建新的根 span
    std::shared_ptr<Span> startSpan(const std::string& name);

    /// @brief 从父 context 创建子 span
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

    std::mt19937_64 m_rng;
    mutable std::mutex m_mutex;
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_TRACING_H