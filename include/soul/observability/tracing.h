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

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_TRACING_H
