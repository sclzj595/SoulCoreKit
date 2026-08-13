// ============================================================================
// middleware.cpp — HTTP Server 中间件链实现 [v2.7.0 增强]
// ============================================================================

#include "soul/server/middleware.h"
#include "soul/logging/log_macros.h"
#include "soul/core/uuid.h"
#include "soul/core/request_context.h"  // v2.8.0

#include <algorithm>
#include <random>
#include <sstream>
#include <iomanip>

namespace sc {
namespace server {

// ============================================================================
// MiddlewareChain 实现
// ============================================================================

void MiddlewareChain::add(std::shared_ptr<IMiddleware> middleware) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_middlewares.push_back(std::move(middleware));
}

const std::vector<std::shared_ptr<IMiddleware>>& MiddlewareChain::middlewares() const noexcept {
    return m_middlewares;
}

void MiddlewareChain::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_middlewares.clear();
}

// ============================================================================
// LoggingMiddleware 实现
// ============================================================================

bool LoggingMiddleware::before(HttpRequest& req, HttpResponse& /*resp*/) {
    std::uint64_t reqId = m_nextRequestId.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(m_timingMutex);
        m_startTimes[reqId] = std::chrono::steady_clock::now();
    }
    req.setHeader("X-Request-Id", QString::number(reqId));
    return true;
}

void LoggingMiddleware::after(const HttpRequest& req, HttpResponse& resp,
                              std::chrono::milliseconds /*duration*/) {
    std::uint64_t reqId = req.header("X-Request-Id").toULongLong();
    std::chrono::steady_clock::time_point startTime;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(m_timingMutex);
        auto it = m_startTimes.find(reqId);
        if (it != m_startTimes.end()) {
            startTime = it->second;
            m_startTimes.erase(it);
            found = true;
        }
    }

    auto elapsed = found
        ? std::chrono::duration_cast<std::chrono::milliseconds>(
              std::chrono::steady_clock::now() - startTime)
        : std::chrono::milliseconds(0);

    SC_INFO("HttpServer: " + std::string(toString(req.method())) + " " +
            req.path().toStdString() + " -> " + std::to_string(resp.status()) +
            " (" + std::to_string(elapsed.count()) + "ms)");
}

// ============================================================================
// TraceMiddleware 实现 [v2.7.0 新增]
// ============================================================================

TraceMiddleware::~TraceMiddleware() = default;

void TraceMiddleware::setTraceIdGenerator(TraceIdGenerator generator) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_traceIdGenerator = std::move(generator);
}

void TraceMiddleware::setHeaderPrefix(const std::string& prefix) {
    m_headerPrefix = prefix;
}

std::string TraceMiddleware::generateTraceId() {
    if (m_traceIdGenerator) {
        return m_traceIdGenerator();
    }
    // 默认: 使用 UUID 字符串（generate() 已返回 std::string）
    return Uuid::generate();
}

std::string TraceMiddleware::generateSpanId() {
    // 生成 16 字符的随机十六进制 span ID
    static thread_local std::mt19937_64 rng(std::random_device{}());
    static thread_local std::uniform_int_distribution<std::uint64_t> dist;

    std::uint64_t val = dist(rng);
    std::ostringstream oss;
    oss << std::hex << std::setfill('0') << std::setw(16) << val;
    return oss.str();
}

bool TraceMiddleware::before(HttpRequest& req, HttpResponse& /*resp*/) {
    // 尝试从请求头提取已有 trace_id
    QString traceparent = req.header(QString::fromStdString(m_headerPrefix + "Trace-Id"));
    QString requestId = req.header("X-Request-Id");

    std::string traceId;
    if (!traceparent.isEmpty()) {
        traceId = traceparent.toStdString();
    } else if (!requestId.isEmpty()) {
        traceId = requestId.toStdString();
    } else {
        traceId = generateTraceId();
    }

    std::string spanId = generateSpanId();

    // 注入 trace 信息到请求头 (供下游服务使用)
    req.setHeader(QString::fromStdString(m_headerPrefix + "Trace-Id"),
                  QString::fromStdString(traceId));
    req.setHeader(QString::fromStdString(m_headerPrefix + "Span-Id"),
                  QString::fromStdString(spanId));

    // v2.8.0: 创建 RequestContext 贯穿本次请求
    auto ctx = RequestContext::fromTrace(
        QString::fromStdString(traceId), "http-server");
    ctx.spanId = QString::fromStdString(spanId);
    ctx.method = QString::fromStdString(toString(req.method()));
    ctx.path = req.path();

    // 将 Context 推入 thread_local 栈 (请求结束时自动清理)
    m_activeContext = std::make_unique<RequestContextGuard>(ctx);

    return true;
}

void TraceMiddleware::after(const HttpRequest& req, HttpResponse& resp,
                            std::chrono::milliseconds duration) {
    // 在响应中返回 trace 信息，方便客户端关联
    QString traceId = req.header(QString::fromStdString(m_headerPrefix + "Trace-Id"));
    QString spanId  = req.header(QString::fromStdString(m_headerPrefix + "Span-Id"));

    if (!traceId.isEmpty()) {
        resp.setHeader(QString::fromStdString(m_headerPrefix + "Trace-Id"), traceId);
    }
    if (!spanId.isEmpty()) {
        resp.setHeader(QString::fromStdString(m_headerPrefix + "Span-Id"), spanId);
    }

    // 记录 span 信息到日志
    SC_INFO("Trace: trace_id=" + traceId.toStdString() +
            " span_id=" + spanId.toStdString() +
            " method=" + std::string(toString(req.method())) +
            " path=" + req.path().toStdString() +
            " status=" + std::to_string(resp.status()) +
            " duration=" + std::to_string(duration.count()) + "ms");

    // v2.8.0: 清理 RequestContext (ScopeGuard 析构)
    m_activeContext.reset();
}

// ============================================================================
// AuthMiddleware 实现
// ============================================================================

AuthMiddleware::AuthMiddleware(TokenValidator validator)
    : m_validator(std::move(validator)) {
}

void AuthMiddleware::addExcludePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_excludePaths.push_back(path);
}

void AuthMiddleware::setUnauthorizedMessage(const std::string& msg) {
    m_unauthorizedMsg = msg;
}

bool AuthMiddleware::before(HttpRequest& req, HttpResponse& resp) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto path = req.path().toStdString();
        bool excluded = std::any_of(m_excludePaths.begin(), m_excludePaths.end(),
            [&path](const std::string& ep) { return path == ep; });
        if (excluded) {
            return true;
        }
    }

    QString authHeader = req.header("Authorization");
    if (authHeader.isEmpty()) {
        resp.setStatus(401);
        resp.setHeader("WWW-Authenticate", "Bearer");
        resp.setBody(QByteArray(m_unauthorizedMsg.c_str()));
        return false;
    }

    QString authStr = authHeader.trimmed();
    if (!authStr.startsWith("Bearer ")) {
        resp.setStatus(401);
        resp.setHeader("WWW-Authenticate", "Bearer");
        resp.setBody(QByteArray(m_unauthorizedMsg.c_str()));
        return false;
    }

    std::string token = authStr.mid(7).toStdString();
    if (token.empty() || !m_validator(token)) {
        resp.setStatus(401);
        resp.setHeader("WWW-Authenticate", "Bearer");
        resp.setBody(QByteArray(m_unauthorizedMsg.c_str()));
        return false;
    }

    return true;
}

void AuthMiddleware::after(const HttpRequest& /*req*/, HttpResponse& /*resp*/,
                           std::chrono::milliseconds /*duration*/) {
}

// ============================================================================
// ValidationMiddleware 实现 [v2.7.0 新增]
// ============================================================================

void ValidationMiddleware::setMaxBodySize(std::size_t bytes) {
    m_maxBodySize = bytes;
}

void ValidationMiddleware::addAllowedContentType(const std::string& contentType) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_allowedContentTypes.push_back(contentType);
}

void ValidationMiddleware::addExcludePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_excludePaths.push_back(path);
}

bool ValidationMiddleware::before(HttpRequest& req, HttpResponse& resp) {
    // 检查排除路径
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto path = req.path().toStdString();
        bool excluded = std::any_of(m_excludePaths.begin(), m_excludePaths.end(),
            [&path](const std::string& ep) { return path == ep; });
        if (excluded) {
            return true;
        }
    }

    // GET/HEAD/DELETE/OPTIONS 无请求体，跳过校验
    auto method = req.method();
    if (method == HttpMethod::Get || method == HttpMethod::Head ||
        method == HttpMethod::Delete || method == HttpMethod::Options) {
        return true;
    }

    // 校验请求体大小
    if (req.body().size() > static_cast<int>(m_maxBodySize)) {
        resp.setStatus(413);  // Payload Too Large
        resp.setBody(QByteArray("Request body too large"));
        return false;
    }

    // 校验 Content-Type
    QString contentType = req.header("Content-Type");
    if (!contentType.isEmpty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        bool allowed = std::any_of(m_allowedContentTypes.begin(), m_allowedContentTypes.end(),
            [&contentType](const std::string& ct) {
                return contentType.contains(QString::fromStdString(ct), Qt::CaseInsensitive);
            });
        if (!allowed) {
            resp.setStatus(415);  // Unsupported Media Type
            resp.setBody(QByteArray("Unsupported Content-Type"));
            return false;
        }
    }

    return true;
}

void ValidationMiddleware::after(const HttpRequest& /*req*/, HttpResponse& /*resp*/,
                                  std::chrono::milliseconds /*duration*/) {
    // after 阶段暂不做响应体校验 (可扩展 Schema 校验)
}

// ============================================================================
// CorsMiddleware 实现
// ============================================================================

CorsMiddleware::CorsMiddleware() = default;

void CorsMiddleware::setAllowedOrigin(const std::string& origin) {
    m_allowedOrigin = origin;
}

void CorsMiddleware::setAllowedMethods(const std::string& methods) {
    m_allowedMethods = methods;
}

void CorsMiddleware::setAllowedHeaders(const std::string& headers) {
    m_allowedHeaders = headers;
}

void CorsMiddleware::setAllowCredentials(bool allow) {
    m_allowCredentials = allow;
}

bool CorsMiddleware::before(HttpRequest& req, HttpResponse& resp) {
    if (req.method() == HttpMethod::Options) {
        resp.setStatus(204);
        resp.setHeader("Access-Control-Allow-Origin",
                       QString::fromStdString(m_allowedOrigin));
        resp.setHeader("Access-Control-Allow-Methods",
                       QString::fromStdString(m_allowedMethods));
        resp.setHeader("Access-Control-Allow-Headers",
                       QString::fromStdString(m_allowedHeaders));
        if (m_allowCredentials) {
            resp.setHeader("Access-Control-Allow-Credentials", "true");
        }
        resp.setHeader("Access-Control-Max-Age", "86400");
        return false;
    }
    return true;
}

void CorsMiddleware::after(const HttpRequest& /*req*/, HttpResponse& resp,
                           std::chrono::milliseconds /*duration*/) {
    resp.setHeader("Access-Control-Allow-Origin",
                   QString::fromStdString(m_allowedOrigin));
    resp.setHeader("Access-Control-Allow-Methods",
                   QString::fromStdString(m_allowedMethods));
    resp.setHeader("Access-Control-Allow-Headers",
                   QString::fromStdString(m_allowedHeaders));
    if (m_allowCredentials) {
        resp.setHeader("Access-Control-Allow-Credentials", "true");
    }
}

// ============================================================================
// RateLimitMiddleware 实现
// ============================================================================

RateLimitMiddleware::RateLimitMiddleware(std::shared_ptr<sc::network::RateLimiter> limiter)
    : m_limiter(std::move(limiter)) {
}

void RateLimitMiddleware::addExcludePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_excludePaths.push_back(path);
}

bool RateLimitMiddleware::before(HttpRequest& req, HttpResponse& resp) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto path = req.path().toStdString();
        bool excluded = std::any_of(m_excludePaths.begin(), m_excludePaths.end(),
            [&path](const std::string& ep) {
                if (path.size() >= ep.size() && path.compare(0, ep.size(), ep) == 0) {
                    return path.size() == ep.size() || path[ep.size()] == '/';
                }
                return false;
            });
        if (excluded) {
            return true;
        }
    }

    if (!m_limiter || !m_limiter->tryAcquire()) {
        resp.setStatus(429);
        resp.setHeader("Retry-After", "1");
        resp.setBody(QByteArray("Too Many Requests"));
        return false;
    }

    return true;
}

void RateLimitMiddleware::after(const HttpRequest& /*req*/, HttpResponse& /*resp*/,
                                std::chrono::milliseconds /*duration*/) {
}

} // namespace server
} // namespace sc
