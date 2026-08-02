#include "soul/observability/tracing.h"

#include <QUuid>
#include <random>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <cctype>

namespace sc {
namespace observability {

// ============================================================================
// ID 生成
// ============================================================================

// ============================================================================
// W3C validation helpers
// ============================================================================

static bool isValidHex(const std::string& s, size_t expectedLen) {
    if (s.size() != expectedLen) return false;
    for (char c : s) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

static bool isAllZero(const std::string& s) {
    return s.find_first_not_of('0') == std::string::npos;
}

std::string Tracer::generateTraceId() {
    // W3C: traceId 为 16 字节(32 字符十六进制)
    QUuid uuid = QUuid::createUuid();
    QString hex = uuid.toString(QUuid::WithoutBraces);
    hex.remove('-');
    return hex.toStdString();
}

std::string Tracer::generateSpanId() {
    // W3C: spanId 为 8 字节(16 字符十六进制)
    static thread_local std::mt19937_64 rng(
        std::random_device{}() ^
        static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count()));

    std::uint64_t id = rng();
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << id;
    return oss.str();
}

// ============================================================================
// W3C Trace Context 解析/格式化
// ============================================================================

SpanContext Tracer::parseTraceparent(const std::string& header) {
    SpanContext ctx;
    // 格式: version-traceid-spanid-flags
    // 示例: 00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01
    if (header.size() != 55) return ctx;

    std::vector<std::string> parts;
    std::istringstream iss(header);
    std::string part;
    while (std::getline(iss, part, '-')) {
        parts.push_back(part);
    }

    if (parts.size() != 4) return ctx;
    if (parts[0] != "00") return ctx;       // 仅支持 version 00
    if (!isValidHex(parts[1], 32) || !isValidHex(parts[2], 16)) return ctx;  // hex validation
    if (isAllZero(parts[1]) || isAllZero(parts[2])) return ctx;  // reject all-zero

    if (parts[3].size() != 2) return ctx;
    ctx.traceId = parts[1];
    ctx.spanId  = parts[2];
    try {
        ctx.sampled = (std::stoul(parts[3], nullptr, 16) & 0x01u) != 0;
    } catch (...) {
        return ctx;
    }
    return ctx;
}

std::string Tracer::formatTraceparent(const SpanContext& context) {
    // W3C traceparent 格式: 00-traceId-spanId-0X
    std::ostringstream oss;
    oss << "00-" << context.traceId << "-" << context.spanId << "-"
        << (context.sampled ? "01" : "00");
    return oss.str();
}

SpanContext Tracer::extractFromHeaders(const QMap<QString, QString>& headers) {
    auto it = headers.find("traceparent");
    if (it != headers.end()) {
        return parseTraceparent(it->toStdString());
    }
    return SpanContext{};
}

void Tracer::injectToHeaders(const std::shared_ptr<Span>& span,
                               QMap<QString, QString>& headers) {
    if (!span) return;
    headers["traceparent"] = QString::fromStdString(
        formatTraceparent(span->context()));
}

// ============================================================================
// Tracer
// ============================================================================

Tracer::Tracer() = default;

Tracer& Tracer::instance() {
    static Tracer inst;
    return inst;
}

std::shared_ptr<Span> Tracer::startSpan(const std::string& name) {
    if (!m_enabled.load(std::memory_order_acquire)) return nullptr;
    SpanContext ctx;
    ctx.traceId = generateTraceId();
    ctx.spanId  = generateSpanId();
    return std::make_shared<Span>(std::move(ctx), name);
}

std::shared_ptr<Span> Tracer::startSpan(const std::string& name,
                                          const SpanContext& parent) {
    if (!m_enabled.load(std::memory_order_acquire)) return nullptr;
    SpanContext ctx;
    ctx.traceId      = parent.traceId;
    ctx.spanId       = generateSpanId();
    ctx.parentSpanId = parent.spanId;
    ctx.sampled      = parent.sampled;
    return std::make_shared<Span>(std::move(ctx), name);
}

} // namespace observability
} // namespace sc