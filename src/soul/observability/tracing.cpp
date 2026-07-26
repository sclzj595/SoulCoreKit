#include "soul/observability/tracing.h"

#include <QUuid>
#include <random>
#include <sstream>
#include <iomanip>

namespace sc {
namespace observability {

// ============================================================================
// TraceContext
// ============================================================================

std::string TraceContext::generateTraceId() {
    // W3C: traceId 为 16 字节（32 字符十六进制）
    // 使用 QUuid 生成 128 位（16 字节）UUID，去掉连字符
    QUuid uuid = QUuid::createUuid();
    QString hex = uuid.toString(QUuid::WithoutBraces);
    hex.remove('-');
    return hex.toStdString();
}

std::string TraceContext::generateSpanId() {
    // W3C: spanId 为 8 字节（16 字符十六进制）
    // 使用随机数生成器
    static thread_local std::mt19937_64 rng(
        std::random_device{}() ^
        static_cast<std::uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count()));

    std::uint64_t id = rng();
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << id;
    return oss.str();
}

std::string TraceContext::toTraceParent() const {
    // W3C traceparent 格式: version-traceid-spanid-flags
    // version: "00"
    // flags: "01"（sampled）
    std::ostringstream oss;
    oss << "00-" << traceId << "-" << spanId << "-01";
    return oss.str();
}

TraceContext TraceContext::fromTraceParent(const std::string& header) {
    TraceContext ctx;
    // 格式: version-traceid-spanid-flags
    // 示例: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
    if (header.size() < 55) return ctx;  // 长度不足

    // 简单解析：按 '-' 分割
    std::vector<std::string> parts;
    std::istringstream iss(header);
    std::string part;
    while (std::getline(iss, part, '-')) {
        parts.push_back(part);
    }

    if (parts.size() != 4) return ctx;
    if (parts[0] != "00") return ctx;
    if (parts[1].size() != 32) return ctx;
    if (parts[2].size() != 16) return ctx;

    ctx.traceId = parts[1];
    ctx.spanId  = parts[2];
    // parts[3] 是 flags，当前忽略
    return ctx;
}

// ============================================================================
// Span
// ============================================================================

Span::Span(std::string name, TraceContext context, SpanKind kind)
    : m_name(std::move(name))
    , m_context(std::move(context))
    , m_kind(kind)
    , m_startTime(std::chrono::steady_clock::now()) {
}

Span::~Span() {
    if (!m_ended.load(std::memory_order_relaxed)) {
        end();
    }
}

void Span::setAttribute(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stringAttributes[key] = value;
}

void Span::setAttribute(const std::string& key, const char* value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stringAttributes[key] = value;
}

void Span::setAttribute(const std::string& key, double value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_numericAttributes[key] = value;
}

void Span::setAttribute(const std::string& key, bool value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stringAttributes[key] = value ? "true" : "false";
}

void Span::addEvent(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.push_back({std::chrono::steady_clock::now(), name, {}});
}

void Span::addEvent(const std::string& name,
                     const std::map<std::string, std::string>& attributes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_events.push_back({std::chrono::steady_clock::now(), name, attributes});
}

void Span::setStatus(SpanStatus status, const std::string& description) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_status = status;
    m_statusDescription = description;
}

void Span::end() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_ended.load(std::memory_order_relaxed)) {
        return;  // 已结束
    }
    m_endTime = std::chrono::steady_clock::now();
    // release 序确保 m_endTime 的写入对 acquire 序的读者可见
    m_ended.store(true, std::memory_order_release);
}

std::chrono::milliseconds Span::duration() const {
    // 先用 acquire 序读 m_ended 做快速路径(未结束时不需持锁)
    if (!m_ended.load(std::memory_order_acquire)) {
        return std::chrono::milliseconds(0);
    }
    // 已结束:持锁读取 m_endTime,保证与 end() 的写入一致
    std::lock_guard<std::mutex> lock(m_mutex);
    return std::chrono::duration_cast<std::chrono::milliseconds>(m_endTime - m_startTime);
}

std::map<std::string, std::string> Span::stringAttributes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stringAttributes;
}

std::map<std::string, double> Span::numericAttributes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_numericAttributes;
}

std::vector<SpanEvent> Span::events() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_events;
}

// ============================================================================
// Tracer
// ============================================================================

Tracer& Tracer::instance() {
    static Tracer inst;
    return inst;
}

std::shared_ptr<Span> Tracer::startSpan(const std::string& name, SpanKind kind) {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return nullptr;
    }

    TraceContext ctx;
    ctx.traceId = TraceContext::generateTraceId();
    ctx.spanId  = TraceContext::generateSpanId();
    // parentSpanId 为空（根 Span）

    auto span = std::make_shared<Span>(name, ctx, kind);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_spans.push_back(span);
    evictIfNeededUnlocked();
    return span;
}

std::shared_ptr<Span> Tracer::startSpan(const std::string& name,
                                          const Span& parent,
                                          SpanKind kind) {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return nullptr;
    }

    TraceContext ctx;
    ctx.traceId      = parent.context().traceId;  // 继承父 traceId
    ctx.spanId       = TraceContext::generateSpanId();
    ctx.parentSpanId = parent.context().spanId;

    auto span = std::make_shared<Span>(name, ctx, kind);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_spans.push_back(span);
    evictIfNeededUnlocked();
    return span;
}

std::shared_ptr<Span> Tracer::startSpan(const std::string& name,
                                          const TraceContext& parentContext,
                                          SpanKind kind) {
    if (!m_enabled.load(std::memory_order_relaxed)) {
        return nullptr;
    }

    TraceContext ctx;
    ctx.traceId      = parentContext.traceId;  // 继承父 traceId
    ctx.spanId       = TraceContext::generateSpanId();
    ctx.parentSpanId = parentContext.spanId;

    auto span = std::make_shared<Span>(name, ctx, kind);

    std::lock_guard<std::mutex> lock(m_mutex);
    m_spans.push_back(span);
    evictIfNeededUnlocked();
    return span;
}

std::vector<std::shared_ptr<Span>> Tracer::endedSpans() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<Span>> result;
    result.reserve(m_spans.size());
    for (const auto& s : m_spans) {
        if (s->isEnded()) {
            result.push_back(s);
        }
    }
    return result;
}

void Tracer::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_spans.clear();
}

} // namespace observability
} // namespace sc
