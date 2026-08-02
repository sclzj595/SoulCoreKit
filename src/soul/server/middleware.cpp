// ============================================================================
// middleware.cpp — HTTP Server 中间件链实现
// ============================================================================

#include "soul/server/middleware.h"
#include "soul/logging/log_macros.h"

#include <algorithm>

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
    // 生成不透明 request ID,存储开始时间到内部 map,避免污染请求头
    std::uint64_t reqId = m_nextRequestId.fetch_add(1, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock(m_timingMutex);
        m_startTimes[reqId] = std::chrono::steady_clock::now();
    }
    // 在请求头中仅携带不透明 ID,不暴露计时数据
    req.setHeader("X-Request-Id", QString::number(reqId));
    return true;
}

void LoggingMiddleware::after(const HttpRequest& req, HttpResponse& resp,
                              std::chrono::milliseconds /*duration*/) {
    // 从请求头提取 request ID,计算耗时
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

    // 提取 Authorization 头
    QString authHeader = req.header("Authorization");
    if (authHeader.isEmpty()) {
        resp.setStatus(401);
        resp.setHeader("WWW-Authenticate", "Bearer");
        resp.setBody(QByteArray(m_unauthorizedMsg.c_str()));
        return false;
    }

    // 解析 Bearer token
    QString authStr = authHeader.trimmed();
    if (!authStr.startsWith("Bearer ")) {
        resp.setStatus(401);
        resp.setHeader("WWW-Authenticate", "Bearer");
        resp.setBody(QByteArray(m_unauthorizedMsg.c_str()));
        return false;
    }

    std::string token = authStr.mid(7).toStdString();  // 跳过 "Bearer "
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
    // AuthMiddleware 在 after 阶段无操作
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
    // OPTIONS 预检请求:直接返回 204,不执行 handler
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
        return false;  // 短路:不执行 handler
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
// RateLimitMiddleware 实现 [v1.9.3]
// ============================================================================

RateLimitMiddleware::RateLimitMiddleware(std::shared_ptr<sc::network::RateLimiter> limiter)
    : m_limiter(std::move(limiter)) {
}

void RateLimitMiddleware::addExcludePath(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_excludePaths.push_back(path);
}

bool RateLimitMiddleware::before(HttpRequest& req, HttpResponse& resp) {
    // 检查排除路径(支持前缀匹配,自动处理尾部斜杠)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto path = req.path().toStdString();
        bool excluded = std::any_of(m_excludePaths.begin(), m_excludePaths.end(),
            [&path](const std::string& ep) {
                if (path.size() >= ep.size() && path.compare(0, ep.size(), ep) == 0) {
                    // 精确匹配 或 前缀匹配且下一个字符是 '/'
                    return path.size() == ep.size() || path[ep.size()] == '/';
                }
                return false;
            });
        if (excluded) {
            return true;
        }
    }

    // 限流检查
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
    // RateLimitMiddleware 在 after 阶段无操作
}

} // namespace server
} // namespace sc