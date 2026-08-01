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
    // 记录开始时间(存入请求属性,供 after 计算耗时)
    req.setHeader("X-Middleware-StartTime",
                  QString::number(std::chrono::steady_clock::now().time_since_epoch().count()));
    return true;
}

void LoggingMiddleware::after(const HttpRequest& req, HttpResponse& resp,
                              std::chrono::milliseconds /*duration*/) {
    // 计算耗时
    auto startNs = req.header("X-Middleware-StartTime").toLongLong();
    auto now = std::chrono::steady_clock::now().time_since_epoch().count();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::nanoseconds(now - startNs));

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

} // namespace server
} // namespace sc