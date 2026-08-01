#ifndef SOUL_SERVER_HTTP_SERVER_H
#define SOUL_SERVER_HTTP_SERVER_H

// ============================================================================
// http_server.h — 嵌入式 HTTP Server(CS 架构 Server 端通信入口)
// ============================================================================
//
// 设计目标: 提供 CS 架构中 Server 端的 HTTP 通信能力。基于 QTcpServer
// 自研轻量实现,无外部依赖(Qt6::Network 已包含),确保 CI 环境可构建。
//
// 设计原则(遵循 project_memory 硬约束):
//   - C++17 严格限定
//   - RAII + 智能指针,严禁裸指针
//   - 单一职责: HttpServer 仅负责监听/分发,不处理业务逻辑
//   - 线程安全: 路由注册/中间件注册/查找线程安全
//   - 最小依赖: 仅依赖 Qt6::Network + soul_core,不引入 QtHttpServer
//   - 跨平台: Windows/Linux/macOS 行为一致
//
// 支持特性:
//   - HTTP/1.1 基础请求解析(GET/POST/PUT/DELETE/HEAD/OPTIONS/PATCH)
//   - 基于路径 + 方法的路由分发
//   - 中间件链(before/after 两阶段,支持短路拒绝) [v1.9.1 新增]
//   - 请求/响应抽象(HttpRequest/HttpResponse)
//   - 连接超时管理
//   - 请求跨 TCP 段缓冲(v1.9.0 修复: 不完整请求不再误返 400)
//   - 不支持 HTTPS/TLS(保持轻量,后续可扩展)
//
// 用法:
//   sc::server::HttpServer server;
//   server.use(std::make_shared<sc::server::LoggingMiddleware>());
//   server.use(std::make_shared<sc::server::AuthMiddleware>(tokenValidator));
//   server.get("/api/health", [](const HttpRequest& req, HttpResponse& resp) {
//       resp.setStatus(200).setBody("{\"status\":\"ok\"}");
//   });
//   server.listen(QHostAddress::Any, 8080);

#include <QHostAddress>
#include <QObject>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QByteArray>
#include <QDateTime>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace sc {
namespace server {

// 前置声明(避免循环依赖)
class IMiddleware;
class MiddlewareChain;

// ============================================================================
// HttpMethod — HTTP 方法枚举
// ============================================================================
enum class HttpMethod {
    Get,
    Post,
    Put,
    Delete,
    Head,
    Options,
    Patch
};

/// @brief HTTP 方法转字符串
inline const char* toString(HttpMethod method) {
    switch (method) {
        case HttpMethod::Get:     return "GET";
        case HttpMethod::Post:    return "POST";
        case HttpMethod::Put:     return "PUT";
        case HttpMethod::Delete:  return "DELETE";
        case HttpMethod::Head:    return "HEAD";
        case HttpMethod::Options: return "OPTIONS";
        case HttpMethod::Patch:   return "PATCH";
    }
    return "UNKNOWN";
}

/// @brief 字符串转 HTTP 方法,失败返回 std::optional 为空
std::optional<HttpMethod> fromString(const std::string& method);

// ============================================================================
// HttpRequest — HTTP 请求抽象
// ============================================================================
class HttpRequest {
public:
    HttpRequest() = default;

    /// @return HTTP 方法
    HttpMethod method() const noexcept { return m_method; }
    void setMethod(HttpMethod m) { m_method = m; }

    /// @return 请求路径(不含 query string)
    const QString& path() const noexcept { return m_path; }
    void setPath(QString p) { m_path = std::move(p); }

    /// @return 原始 URI(含 query string)
    const QString& uri() const noexcept { return m_uri; }
    void setUri(QString u) { m_uri = std::move(u); }

    /// @return 查询参数(key -> value)
    const QMap<QString, QString>& queryParams() const noexcept { return m_queryParams; }
    void setQueryParams(QMap<QString, QString> p) { m_queryParams = std::move(p); }

    /// @return 请求头(key -> value,key 已转小写)
    const QMap<QString, QString>& headers() const noexcept { return m_headers; }
    void setHeader(const QString& key, const QString& value) {
        m_headers.insert(key.toLower(), value);
    }
    QString header(const QString& key) const {
        return m_headers.value(key.toLower());
    }

    /// @return 请求体
    const QByteArray& body() const noexcept { return m_body; }
    void setBody(QByteArray b) { m_body = std::move(b); }

    /// @return 客户端地址
    QHostAddress peerAddress() const noexcept { return m_peer; }
    void setPeerAddress(QHostAddress addr) { m_peer = std::move(addr); }

private:
    HttpMethod              m_method = HttpMethod::Get;
    QString                 m_path;
    QString                 m_uri;
    QMap<QString, QString>  m_queryParams;
    QMap<QString, QString>  m_headers;
    QByteArray              m_body;
    QHostAddress            m_peer;
};

// ============================================================================
// HttpResponse — HTTP 响应抽象
// ============================================================================
class HttpResponse {
public:
    HttpResponse() = default;

    /// @return 状态码
    int status() const noexcept { return m_status; }
    void setStatus(int code) { m_status = code; }

    /// @return 响应头
    const QMap<QString, QString>& headers() const noexcept { return m_headers; }
    void setHeader(const QString& key, const QString& value) {
        m_headers.insert(key, value);
    }

    /// @return 响应体
    const QByteArray& body() const noexcept { return m_body; }
    void setBody(QByteArray b) { m_body = std::move(b); }
    void setBody(const QString& text) { m_body = text.toUtf8(); }
    void setBody(const char* text) { m_body = QByteArray(text); }

    /// @brief 序列化为 HTTP/1.1 响应字节流
    QByteArray serialize() const;

private:
    /// @brief 状态码转原因短语
    static const char* statusText(int code);

    int                     m_status = 200;
    QMap<QString, QString>  m_headers;
    QByteArray              m_body;
};

// ============================================================================
// RouteHandler — 路由处理函数类型
// ============================================================================
using RouteHandler = std::function<void(const HttpRequest&, HttpResponse&)>;

// ============================================================================
// HttpServer — 嵌入式 HTTP Server
// ============================================================================
//
// 基于 QTcpServer 的轻量 HTTP/1.1 Server。支持路由分发、连接超时管理。
//
// @thread_safety 路由注册/查找线程安全。请求处理在 QTcpServer 所在线程。
class HttpServer : public QObject {
    Q_OBJECT
public:
    /// @param parent Qt 父对象
    explicit HttpServer(QObject* parent = nullptr);
    ~HttpServer() override;

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    /// @brief 开始监听指定端口
    /// @param address 监听地址(默认 Any=0.0.0.0)
    /// @param port     监听端口
    /// @return 是否成功
    bool listen(const QHostAddress& address = QHostAddress::Any, quint16 port = 0);

    /// @brief 停止监听并关闭所有连接(立即关闭)
    void close();

    /// @brief 优雅关闭 [v1.9.3 新增]
    /// @param gracePeriodMs 等待已完成请求的最长时间(毫秒,默认 5000)
    /// @note 1. 停止接受新连接,返回 503 给新连接
    ///       2. 等待 in-flight 请求完成(最多 gracePeriodMs)
    ///       3. 超时后强制关闭所有连接
    void shutdown(int gracePeriodMs = 5000);

    /// @return 是否正在优雅关闭
    bool isShuttingDown() const noexcept;

    /// @return 当前正在处理的请求数
    int inFlightRequests() const noexcept;

    /// @return 是否正在监听
    bool isListening() const noexcept;

    /// @return 实际监听端口(listen 时 port=0 自动分配时有用)
    quint16 serverPort() const noexcept;

    /// @brief 注册路由
    /// @param method  HTTP 方法
    /// @param path    路径(精确匹配,如 "/api/health")
    /// @param handler 处理函数
    void route(HttpMethod method, const QString& path, RouteHandler handler);

    /// @brief 注册 GET 路由(便捷方法)
    void get(const QString& path, RouteHandler handler) {
        route(HttpMethod::Get, path, std::move(handler));
    }

    /// @brief 注册 POST 路由(便捷方法)
    void post(const QString& path, RouteHandler handler) {
        route(HttpMethod::Post, path, std::move(handler));
    }

    /// @brief 注册 PUT 路由(便捷方法)
    void put(const QString& path, RouteHandler handler) {
        route(HttpMethod::Put, path, std::move(handler));
    }

    /// @brief 注册 DELETE 路由(便捷方法)
    void del(const QString& path, RouteHandler handler) {
        route(HttpMethod::Delete, path, std::move(handler));
    }

    /// @brief 设置默认 404 处理器
    void setNotFoundHandler(RouteHandler handler);

    /// @brief 设置连接超时(毫秒,默认 30000)
    void setConnectionTimeout(int timeoutMs);

    /// @brief 设置最大并发连接数(默认 0 表示不限制) [v1.9.1 新增]
    /// @param max 最大连接数,0 表示不限制
    void setMaxConnections(int max);

    /// @return 最大并发连接数(0 表示不限制)
    int maxConnections() const noexcept;

    /// @return 当前活跃连接数
    int currentConnections() const noexcept;

    /// @brief 注册中间件(添加到中间件链尾部) [v1.9.1 新增]
    /// @param middleware 中间件实例(shared_ptr,HttpServer 不拥有所有权)
    /// @return *this 以支持链式调用
    /// @note 中间件按注册顺序执行 Before,按逆序执行 After
    HttpServer& use(std::shared_ptr<IMiddleware> middleware);

    /// @brief 获取中间件链引用(用于高级场景,如动态添加/移除中间件)
    MiddlewareChain& middlewareChain() noexcept;

signals:
    /// @brief 新连接到达
    void connectionAccepted(const QHostAddress& peer);

    /// @brief 请求处理完成
    void requestHandled(const QString& method, const QString& path, int status);

    /// @brief 连接数达到上限时拒绝连接 [v1.9.1 新增]
    /// @param peer 被拒绝的客户端地址
    void connectionLimitReached(const QHostAddress& peer);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    // 路由键 = "METHOD /path"
    struct RouteKey {
        HttpMethod method;
        QString    path;
        bool operator==(const RouteKey& other) const {
            return method == other.method && path == other.path;
        }
    };
    struct RouteKeyHash {
        std::size_t operator()(const RouteKey& k) const {
            return std::hash<int>()(static_cast<int>(k.method))
                 ^ (std::hash<QString>()(k.path) << 1);
        }
    };

    using RouteMap = std::unordered_map<RouteKey, RouteHandler, RouteKeyHash>;

    /// @brief 请求解析状态
    enum class ParseStatus {
        Ok,         ///< 解析成功,请求完整
        Incomplete, ///< 数据不完整,需等待更多数据(v1.9.0: 不再误返 400)
        BadRequest  ///< 请求格式错误
    };

    /// @brief 解析 HTTP 请求字节流
    /// @return 解析状态(Ok/Incomplete/BadRequest)
    ParseStatus parseRequest(QTcpSocket* socket, HttpRequest& req, const QByteArray& data);

    /// @brief 查找匹配的路由处理器
    RouteHandler findRoute(HttpMethod method, const QString& path) const;

    /// @brief 发送响应并关闭连接
    void sendResponse(QTcpSocket* socket, const HttpResponse& response);

    QTcpServer*                 m_tcpServer = nullptr;
    mutable std::mutex          m_routeMutex;
    RouteMap                    m_routes;
    RouteHandler                m_notFoundHandler;
    std::atomic<int>            m_connectionTimeoutMs{30000};  ///< v1.9.0: atomic 防止 setConnectionTimeout/onNewConnection 数据竞争
    std::atomic<int>            m_maxConnections{0};          ///< v1.9.1: 最大并发连接数,0=不限制
    std::atomic<int>            m_currentConnections{0};      ///< v1.9.1: 当前活跃连接数
    // v1.9.0: per-socket 请求缓冲,支持 HTTP 请求跨 TCP 段到达
    mutable std::mutex          m_bufferMutex;
    std::unordered_map<QTcpSocket*, QByteArray> m_buffers;
    // v1.9.1: 中间件链
    std::unique_ptr<MiddlewareChain> m_middlewareChain;
    // v1.9.3: 优雅关闭
    std::atomic<bool> m_shuttingDown{false};
    std::atomic<int>  m_inFlightRequests{0};
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_HTTP_SERVER_H
