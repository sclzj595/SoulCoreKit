// ============================================================================
// request_context.cpp — RequestContext 实现 [v2.8.0]
// ============================================================================

#include "soul/core/request_context.h"
#include "soul/core/uuid.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <vector>

namespace sc {

// ============================================================================
// thread_local 栈 — 支持嵌套 Context (子 Span)
// ============================================================================

namespace {
    thread_local std::vector<RequestContext> tls_contextStack;
}

// ============================================================================
// RequestContext 实现
// ============================================================================

RequestContext RequestContext::create(const QString& sourceComponent) {
    RequestContext ctx;
    ctx.requestId = QString::fromStdString(Uuid::generate());
    ctx.traceId = QString::fromStdString(Uuid::generate());
    ctx.spanId = generateSpanId();
    ctx.sourceComponent = sourceComponent;
    ctx.startTime = std::chrono::steady_clock::now();
    return ctx;
}

RequestContext RequestContext::fromTrace(const QString& traceId,
                                          const QString& sourceComponent) {
    RequestContext ctx;
    ctx.requestId = QString::fromStdString(Uuid::generate());
    ctx.traceId = traceId;
    ctx.spanId = generateSpanId();
    ctx.correlationId = traceId;  // 关联到上游 trace
    ctx.sourceComponent = sourceComponent;
    ctx.startTime = std::chrono::steady_clock::now();
    return ctx;
}

QString RequestContext::generateSpanId() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    static thread_local std::uniform_int_distribution<uint64_t> dist;

    uint64_t val = dist(rng);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << val;
    return QString::fromStdString(oss.str());
}

int64_t RequestContext::elapsedMs() const {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
}

// ============================================================================
// RequestContextGuard 实现
// ============================================================================

RequestContextGuard::RequestContextGuard(const RequestContext& ctx) {
    tls_contextStack.push_back(ctx);
}

RequestContextGuard::~RequestContextGuard() {
    if (!m_popped) {
        if (!tls_contextStack.empty()) {
            tls_contextStack.pop_back();
        }
        m_popped = true;
    }
}

const RequestContext& RequestContextGuard::current() {
    static thread_local RequestContext empty;
    if (tls_contextStack.empty()) {
        return empty;
    }
    return tls_contextStack.back();
}

bool RequestContextGuard::hasCurrent() {
    return !tls_contextStack.empty();
}

QString RequestContextGuard::currentRequestId() {
    if (tls_contextStack.empty()) return {};
    return tls_contextStack.back().requestId;
}

QString RequestContextGuard::currentTraceId() {
    if (tls_contextStack.empty()) return {};
    return tls_contextStack.back().traceId;
}

} // namespace sc
