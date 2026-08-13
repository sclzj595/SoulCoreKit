#ifndef SOUL_SERVER_MIDDLEWARE_H
#define SOUL_SERVER_MIDDLEWARE_H

// ============================================================================
// middleware.h — HTTP Server 中间件链 [v2.7.0 增强]
// ============================================================================
//
// 设计目标: 对标 SpringBoot Filter/Interceptor 链，提供请求处理前后的
// 横切关注点拦截能力 (鉴权/日志/CORS/限流/追踪/校验)。
//
// 执行流程:
//   Request
//      ↓
//   LoggingMiddleware    (before: 生成 request_id)
//      ↓
//   TraceMiddleware      (before: 提取/注入 trace_id)      [v2.7.0 新增]
//      ↓
//   AuthMiddleware       (before: Token 鉴权)
//      ↓
//   RateLimitMiddleware   (before: 限流检查)
//      ↓
//   ValidationMiddleware  (before: 请求体校验)              [v2.7.0 新增]
//      ↓
//   CorsMiddleware       (before: OPTIONS 预检短路)
//      ↓
//   Controller / Handler
//      ↓
//   CorsMiddleware       (after: 添加 CORS 头, 逆序)
//      ↓
//   ValidationMiddleware  (after: 响应体校验)               [v2.7.0 新增]
//      ↓
//   RateLimitMiddleware   (after: 空操作)
//      ↓
//   AuthMiddleware       (after: 空操作)
//      ↓
//   TraceMiddleware      (after: 记录 span)                [v2.7.0 新增]
//      ↓
//   LoggingMiddleware    (after: 记录耗时/状态码, 逆序)
//      ↓
//   Response
//
// 设计原则:
//   - 单一职责: 中间件仅负责横切关注点，不处理业务逻辑
//   - 线程安全: 中间件注册/查找线程安全
//   - 向前兼容: 不影响现有路由注册和使用方式
//   - v2.7.0: 新增 TraceMiddleware + ValidationMiddleware
//
// 用法:
//   auto& server = HttpServer::instance();
//   server.use(std::make_shared<LoggingMiddleware>());
//   server.use(std::make_shared<TraceMiddleware>());           // v2.7.0
//   server.use(std::make_shared<AuthMiddleware>(validator));
//   server.use(std::make_shared<RateLimitMiddleware>(limiter));
//   server.use(std::make_shared<ValidationMiddleware>());      // v2.7.0
//   server.use(std::make_shared<CorsMiddleware>());
//
// 内置中间件 (6 个):
//   - LoggingMiddleware:     请求日志 (方法/路径/状态码/耗时)
//   - TraceMiddleware:       分布式追踪 (trace_id/span_id)    [v2.7.0 新增]
//   - AuthMiddleware:        Token 鉴权 (可配置排除路径)
//   - RateLimitMiddleware:   限流 (令牌桶/滑动窗口)
//   - ValidationMiddleware:  请求/响应校验                    [v2.7.0 新增]
//   - CorsMiddleware:        CORS 跨域头

#include "soul/server/http_server.h"
#include "soul/network/policy/rate_limiter.h"
#include "soul/core/request_context.h"  // v3.0.0: RequestContextGuard 完整定义

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>  // unique_ptr
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace sc {
namespace server {

// ============================================================================
// IMiddleware — 中间件接口
// ============================================================================
//
// 中间件在请求处理前后执行。Before 阶段可短路拒绝请求 (返回 false)。
// After 阶段始终执行 (即使 Before 短路或 Handler 异常)。
//
// @thread_safety 中间件实例应线程安全，因多个请求可能并发访问。
class IMiddleware {
public:
    virtual ~IMiddleware() = default;

    /// @brief 请求处理前回调
    /// @param req  请求对象 (可修改，如解析 token 后注入用户信息)
    /// @param resp 响应对象 (可修改，如设置 CORS 头)
    /// @return true 继续处理; false 短路，不再执行后续中间件和 Handler
    /// @note 返回 false 时应在 resp 中设置合适的错误状态码和消息
    virtual bool before(HttpRequest& req, HttpResponse& resp) = 0;

    /// @brief 请求处理后回调 (始终执行，即使 Handler 异常或 Before 短路)
    /// @param req      请求对象 (只读)
    /// @param resp     响应对象 (可修改，如添加响应头)
    /// @param duration 请求处理耗时
    virtual void after(const HttpRequest& req, HttpResponse& resp,
                       std::chrono::milliseconds duration) = 0;

    /// @return 中间件名称 (用于日志/调试)
    virtual std::string name() const = 0;
};

// ============================================================================
// MiddlewareChain — 中间件链管理器
// ============================================================================
//
// 管理已注册的中间件列表，提供链式执行能力。
// 线程安全: 注册操作加锁，执行时不加锁 (执行期间不应注册新中间件)。
//
// @thread_safety 注册线程安全，执行单线程安全
class MiddlewareChain {
public:
    /// @brief 注册中间件 (添加到链尾)
    void add(std::shared_ptr<IMiddleware> middleware);

    /// @brief 获取已注册中间件列表 (只读快照，供执行使用)
    const std::vector<std::shared_ptr<IMiddleware>>& middlewares() const noexcept;

    /// @brief 清空中间件 (仅用于测试)
    void clear();

private:
    mutable std::mutex m_mutex;
    std::vector<std::shared_ptr<IMiddleware>> m_middlewares;
};

// ============================================================================
// LoggingMiddleware — 请求日志中间件
// ============================================================================
//
// 记录每个请求的方法/路径/状态码/耗时到日志。
// 计时数据存储在内部 map 中，请求头仅携带不透明 request ID，
// 不污染 handler 可见的请求头数据。
class LoggingMiddleware : public IMiddleware {
public:
    bool before(HttpRequest& req, HttpResponse& resp) override;
    void after(const HttpRequest& req, HttpResponse& resp,
               std::chrono::milliseconds duration) override;
    std::string name() const override { return "LoggingMiddleware"; }

private:
    std::atomic<std::uint64_t> m_nextRequestId{0};
    std::unordered_map<std::uint64_t, std::chrono::steady_clock::time_point> m_startTimes;
    std::mutex m_timingMutex;
};

// ============================================================================
// TraceMiddleware — 分布式追踪中间件 [v2.7.0 新增]
// ============================================================================
//
// 从请求头提取或生成 trace_id + span_id，注入响应头。
// 支持 W3C TraceContext (traceparent) 和 B3 格式。
// 与 Observability 模块的 Tracer/Span 集成。
//
// 用法:
//   auto trace = std::make_shared<TraceMiddleware>();
//   server.use(trace);
//
// 请求头:
//   traceparent: 00-{trace_id}-{span_id}-01
//   X-Request-Id: {request_id} (兼容旧格式)
//
// 响应头:
//   X-Trace-Id: {trace_id}
//   X-Span-Id:  {span_id}
class TraceMiddleware : public IMiddleware {
public:
    /// @brief 设置 trace_id 生成器 (默认使用 UUID v4)
    using TraceIdGenerator = std::function<std::string()>;

    TraceMiddleware() = default;
    ~TraceMiddleware() override;  // v3.0.0: 显式析构，解决 unique_ptr<RequestContextGuard> 不完整类型

    void setTraceIdGenerator(TraceIdGenerator generator);

    /// @brief 设置 header 名称前缀 (默认 "X-")
    void setHeaderPrefix(const std::string& prefix);

    bool before(HttpRequest& req, HttpResponse& resp) override;
    void after(const HttpRequest& req, HttpResponse& resp,
               std::chrono::milliseconds duration) override;
    std::string name() const override { return "TraceMiddleware"; }

private:
    std::string generateTraceId();
    std::string generateSpanId();
    TraceIdGenerator m_traceIdGenerator;
    std::string m_headerPrefix = "X-";
    std::mutex m_mutex;
    std::unique_ptr<RequestContextGuard> m_activeContext;  // v2.8.0: Context 生命周期
};

// ============================================================================
// AuthMiddleware — Token 鉴权中间件
// ============================================================================
//
// 从 Authorization 头提取 Bearer token，调用验证器检查。
// 可配置排除路径 (如 /api/health 不校验)。
//
// 用法:
//   auto auth = std::make_shared<AuthMiddleware>(
//       [](const std::string& token) { return token == "valid-token"; });
//   auth->addExcludePath("/api/health");
//   server.use(auth);
class AuthMiddleware : public IMiddleware {
public:
    using TokenValidator = std::function<bool(const std::string& token)>;

    /// @param validator Token 验证器，返回 true 表示有效
    explicit AuthMiddleware(TokenValidator validator);

    /// @brief 添加排除路径 (不进行鉴权检查)
    void addExcludePath(const std::string& path);

    /// @brief 设置鉴权失败时的错误消息 (默认 "Unauthorized")
    void setUnauthorizedMessage(const std::string& msg);

    bool before(HttpRequest& req, HttpResponse& resp) override;
    void after(const HttpRequest& req, HttpResponse& resp,
               std::chrono::milliseconds duration) override;
    std::string name() const override { return "AuthMiddleware"; }

private:
    TokenValidator           m_validator;
    std::vector<std::string> m_excludePaths;
    std::string              m_unauthorizedMsg = "Unauthorized";
    mutable std::mutex       m_mutex;
};

// ============================================================================
// ValidationMiddleware — 请求/响应校验中间件 [v2.7.0 新增]
// ============================================================================
//
// 在 before 阶段校验请求体 (Content-Type, Size, Schema)，
// 在 after 阶段校验响应体。
// 可与 soul_validation 模块集成。
//
// 用法:
//   auto validation = std::make_shared<ValidationMiddleware>();
//   validation->setMaxBodySize(1024 * 1024);  // 1MB
//   validation->addAllowedContentType("application/json");
//   server.use(validation);
class ValidationMiddleware : public IMiddleware {
public:
    /// @brief 设置最大请求体大小 (字节)，默认 1MB
    void setMaxBodySize(std::size_t bytes);

    /// @brief 添加允许的 Content-Type
    void addAllowedContentType(const std::string& contentType);

    /// @brief 添加排除路径 (不进行校验)
    void addExcludePath(const std::string& path);

    bool before(HttpRequest& req, HttpResponse& resp) override;
    void after(const HttpRequest& req, HttpResponse& resp,
               std::chrono::milliseconds duration) override;
    std::string name() const override { return "ValidationMiddleware"; }

private:
    std::size_t m_maxBodySize = 1024 * 1024;  // 1MB default
    std::vector<std::string> m_allowedContentTypes = {"application/json"};
    std::vector<std::string> m_excludePaths;
    mutable std::mutex m_mutex;
};

// ============================================================================
// CorsMiddleware — CORS 跨域中间件
// ============================================================================
//
// 为 HTTP 响应添加 CORS 头，主要用于 CS 架构中浏览器调试工具访问。
// 默认允许所有来源 (开发环境)，生产环境可配置具体来源。
//
// 用法:
//   auto cors = std::make_shared<CorsMiddleware>();
//   cors->setAllowedOrigin("https://admin.example.com");
//   server.use(cors);
class CorsMiddleware : public IMiddleware {
public:
    CorsMiddleware();

    /// @brief 设置允许的来源 (默认 "*")
    void setAllowedOrigin(const std::string& origin);

    /// @brief 设置允许的方法 (默认 "GET,POST,PUT,DELETE,OPTIONS")
    void setAllowedMethods(const std::string& methods);

    /// @brief 设置允许的请求头 (默认 "Content-Type,Authorization")
    void setAllowedHeaders(const std::string& headers);

    /// @brief 设置是否允许携带凭证 (默认 true)
    void setAllowCredentials(bool allow);

    bool before(HttpRequest& req, HttpResponse& resp) override;
    void after(const HttpRequest& req, HttpResponse& resp,
               std::chrono::milliseconds duration) override;
    std::string name() const override { return "CorsMiddleware"; }

private:
    std::string m_allowedOrigin = "*";
    std::string m_allowedMethods = "GET,POST,PUT,DELETE,OPTIONS";
    std::string m_allowedHeaders = "Content-Type,Authorization";
    bool        m_allowCredentials = true;
};

// ============================================================================
// RateLimitMiddleware — 限流中间件
// ============================================================================
//
// 基于 RateLimiter (令牌桶/滑动窗口) 的 HTTP 限流中间件。
// 被限流的请求返回 429 Too Many Requests。
//
// 用法:
//   auto limiter = std::make_shared<sc::network::RateLimiter>(
//       sc::network::RateLimiter::Algorithm::TokenBucket, 100.0);
//   server.use(std::make_shared<RateLimitMiddleware>(limiter));
//
// @thread_safety 线程安全 — RateLimiter 本身线程安全
class RateLimitMiddleware : public IMiddleware {
public:
    /// @param limiter 限流器实例 (共享，可被多个中间件共用)
    explicit RateLimitMiddleware(std::shared_ptr<sc::network::RateLimiter> limiter);

    /// @brief 添加排除路径 (不进行限流检查)
    void addExcludePath(const std::string& path);

    bool before(HttpRequest& req, HttpResponse& resp) override;
    void after(const HttpRequest& req, HttpResponse& resp,
               std::chrono::milliseconds duration) override;
    std::string name() const override { return "RateLimitMiddleware"; }

private:
    std::shared_ptr<sc::network::RateLimiter> m_limiter;
    std::vector<std::string> m_excludePaths;
    mutable std::mutex m_mutex;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_MIDDLEWARE_H
