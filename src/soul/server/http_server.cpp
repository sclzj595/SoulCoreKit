// ============================================================================
// http_server.cpp — 嵌入式 HTTP Server 实现
// ============================================================================
//
// 基于 QTcpServer 的轻量 HTTP/1.1 Server 实现。
// 请求解析遵循 RFC 7230(HTTP/1.1)基础语法。

#include "soul/server/http_server.h"
#include "soul/server/middleware.h"
#include "soul/logging/log_macros.h"

#include <QEventLoop>
#include <QUrl>
#include <QUrlQuery>
#include <QTimer>
#include <QThread>
#include <QDateTime>

namespace sc {
namespace server {

// ============================================================================
// 工具函数
// ============================================================================

std::optional<HttpMethod> fromString(const std::string& method) {
    if (method == "GET")     return HttpMethod::Get;
    if (method == "POST")    return HttpMethod::Post;
    if (method == "PUT")     return HttpMethod::Put;
    if (method == "DELETE")  return HttpMethod::Delete;
    if (method == "HEAD")    return HttpMethod::Head;
    if (method == "OPTIONS") return HttpMethod::Options;
    if (method == "PATCH")   return HttpMethod::Patch;
    return std::nullopt;
}

// ============================================================================
// HttpResponse 实现
// ============================================================================

QByteArray HttpResponse::serialize() const {
    QByteArray buf;
    buf.reserve(256 + m_body.size());

    // 状态行
    buf.append("HTTP/1.1 ");
    buf.append(QByteArray::number(m_status));
    buf.append(" ");
    buf.append(statusText(m_status));
    buf.append("\r\n");

    // 响应头(Content-Length 自动补充)
    bool hasContentLength = false;
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
        if (it.key().compare("Content-Length", Qt::CaseInsensitive) == 0) {
            hasContentLength = true;
        }
        buf.append(it.key().toUtf8());
        buf.append(": ");
        buf.append(it.value().toUtf8());
        buf.append("\r\n");
    }
    if (!hasContentLength) {
        buf.append("Content-Length: ");
        buf.append(QByteArray::number(m_body.size()));
        buf.append("\r\n");
    }
    bool hasConnection = false;
    bool hasDate = false;
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
        if (!hasConnection && it.key().compare("Connection", Qt::CaseInsensitive) == 0) {
            hasConnection = true;
        }
        if (!hasDate && it.key().compare("Date", Qt::CaseInsensitive) == 0) {
            hasDate = true;
        }
        if (hasConnection && hasDate) break;
    }
    if (!hasConnection) {
        buf.append("Connection: close\r\n");
    }

    // RFC 7231: 源服务器必须在响应中包含 Date 头 [H-M8]
    if (!hasDate) {
        buf.append("Date: ");
        buf.append(QDateTime::currentDateTimeUtc().toString(Qt::RFC2822Date).toUtf8());
        buf.append("\r\n");
    }

    // 空行 + body
    buf.append("\r\n");
    buf.append(m_body);
    return buf;
}

// 状态码 → 原因短语(RFC 7231 常用子集)
const char* HttpResponse::statusText(int code) {
    switch (code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 304: return "Not Modified";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 409: return "Conflict";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default:  return "Unknown";
    }
}

// ============================================================================
// HttpServer 实现
// ============================================================================

HttpServer::HttpServer(QObject* parent)
    : QObject(parent)
    , m_middlewareChain(std::make_unique<MiddlewareChain>()) {
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &HttpServer::onNewConnection);
}

HttpServer::~HttpServer() {
    close();
}

bool HttpServer::listen(const QHostAddress& address, quint16 port) {
    if (!m_tcpServer->listen(address, port)) {
        SC_ERROR("HttpServer: failed to listen on " +
                 address.toString().toStdString() + ":" +
                 std::to_string(port) + " - " +
                 m_tcpServer->errorString().toStdString());
        return false;
    }
    SC_INFO("HttpServer: listening on " + address.toString().toStdString() +
            ":" + std::to_string(m_tcpServer->serverPort()));
    return true;
}

void HttpServer::close() {
    if (m_tcpServer) {
        m_tcpServer->close();
    }
    // [v1.9.2] 关闭所有已连接的客户端 socket
    // m_tcpServer->close() 仅停止监听,不会断开已有连接
    auto sockets = findChildren<QTcpSocket*>();
    for (auto* socket : sockets) {
        socket->close();
    }
    // 清理所有 per-socket 请求缓冲,避免析构时 socket 访问已销毁的缓冲映射
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffers.clear();
    }
}

void HttpServer::shutdown(int gracePeriodMs) {
    SC_INFO("HttpServer: starting graceful shutdown, grace period = " +
            std::to_string(gracePeriodMs) + "ms");

    // 1. 标记为关闭中,停止接受新连接
    m_shuttingDown.store(true, std::memory_order_release);
    if (m_tcpServer) {
        m_tcpServer->pauseAccepting();
    }

    // 2. 等待 in-flight 请求完成
    // 使用 QEventLoop 而非 QThread::msleep(),确保事件循环在等待期间仍能处理
    // in-flight 请求(onReadyRead 槽函数),避免主线程阻塞导致优雅关闭永远超时。
    if (gracePeriodMs > 0) {
        const int pollIntervalMs = 100;
        int elapsed = 0;
        while (elapsed < gracePeriodMs) {
            int inFlight = m_inFlightRequests.load(std::memory_order_acquire);
            if (inFlight == 0) {
                SC_INFO("HttpServer: all in-flight requests completed in " +
                        std::to_string(elapsed) + "ms");
                break;
            }

            // 嵌套事件循环: 处理待处理事件(包括 onReadyRead/onDisconnected),
            // 同时通过 QTimer 确保超时控制
            QEventLoop loop;
            QTimer::singleShot(pollIntervalMs, &loop, &QEventLoop::quit);
            loop.exec();
            elapsed += pollIntervalMs;
        }

        int remaining = m_inFlightRequests.load(std::memory_order_acquire);
        if (remaining > 0) {
            SC_WARN("HttpServer: grace period expired, forcing close with " +
                    std::to_string(remaining) + " in-flight requests");
        }
    }

    // 3. 关闭所有连接
    close();

    SC_INFO("HttpServer: graceful shutdown completed");
}

bool HttpServer::isShuttingDown() const noexcept {
    return m_shuttingDown.load(std::memory_order_acquire);
}

int HttpServer::inFlightRequests() const noexcept {
    return m_inFlightRequests.load(std::memory_order_acquire);
}

bool HttpServer::isListening() const noexcept {
    return m_tcpServer && m_tcpServer->isListening();
}

quint16 HttpServer::serverPort() const noexcept {
    return m_tcpServer ? m_tcpServer->serverPort() : 0;
}

void HttpServer::route(HttpMethod method, const QString& path, RouteHandler handler) {
    std::lock_guard<std::mutex> lock(m_routeMutex);
    // v2.7.0: 检测路径参数 (:id) 和通配符 (*path)
    if (path.contains(':') || path.contains('*')) {
        m_patternRoutes.push_back({{method, path}, std::move(handler)});
    } else {
        m_routes[{method, path}] = std::move(handler);
    }
}

void HttpServer::setNotFoundHandler(RouteHandler handler) {
    std::lock_guard<std::mutex> lock(m_routeMutex);
    m_notFoundHandler = std::move(handler);
}

void HttpServer::setConnectionTimeout(int timeoutMs) {
    m_connectionTimeoutMs.store(timeoutMs);
}

void HttpServer::setMaxConnections(int max) {
    m_maxConnections.store(max);
}

int HttpServer::maxConnections() const noexcept {
    return m_maxConnections.load();
}

int HttpServer::currentConnections() const noexcept {
    return m_currentConnections.load();
}

HttpServer& HttpServer::use(std::shared_ptr<IMiddleware> middleware) {
    m_middlewareChain->add(std::move(middleware));
    return *this;
}

MiddlewareChain& HttpServer::middlewareChain() noexcept {
    return *m_middlewareChain;
}

std::vector<RouteMapping> HttpServer::getRoutes() const {
    std::lock_guard<std::mutex> lock(m_routeMutex);
    std::vector<RouteMapping> result;
    result.reserve(m_routes.size());
    for (const auto& [key, _] : m_routes) {
        result.push_back({toString(key.method), key.path.toStdString()});
    }
    return result;
}

void HttpServer::onNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* socket = m_tcpServer->nextPendingConnection();
        if (!socket) {
            continue;
        }

        // v1.9.3: 优雅关闭中,拒绝新连接
        if (m_shuttingDown.load(std::memory_order_acquire)) {
            HttpResponse resp;
            resp.setStatus(503);
            resp.setBody(QByteArray("Service Unavailable - Server is shutting down"));
            resp.setHeader("Connection", "close");
            resp.setHeader("Retry-After", "5");
            sendResponse(socket, resp);
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
            continue;
        }

        // v1.9.1: 连接数限制检查
        int maxConns = m_maxConnections.load();
        if (maxConns > 0) {
            int current = m_currentConnections.load();
            if (current >= maxConns) {
                emit connectionLimitReached(socket->peerAddress());
                // 发送 503 响应后关闭连接
                HttpResponse resp;
                resp.setStatus(503);
                resp.setBody(QByteArray("Service Unavailable - Connection limit reached"));
                resp.setHeader("Retry-After", "5");
                sendResponse(socket, resp);
                connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
                continue;
            }
        }

        m_currentConnections.fetch_add(1);
        socket->setParent(this);
        emit connectionAccepted(socket->peerAddress());

        // 连接超时定时器(属性方式附加到 socket)
        socket->setProperty("connectTime", QDateTime::currentDateTime());

        connect(socket, &QTcpSocket::readyRead, this, &HttpServer::onReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &HttpServer::onDisconnected);

        // 设置超时:超时后自动关闭
        QTimer::singleShot(m_connectionTimeoutMs.load(), socket, [socket]() {
            if (socket->state() != QAbstractSocket::UnconnectedState) {
                socket->close();
            }
        });
    }
}

void HttpServer::onReadyRead() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }

    // v1.9.3: 追踪 in-flight 请求(RAII 自动递减)
    m_inFlightRequests.fetch_add(1, std::memory_order_release);
    struct InFlightGuard {
        std::atomic<int>& counter;
        ~InFlightGuard() { counter.fetch_sub(1, std::memory_order_release); }
    } guard{m_inFlightRequests};

    // 累积本次到达的数据到 per-socket 缓冲(支持 HTTP 请求跨 TCP 段到达)
    bool tooLarge = false;
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffers[socket].append(socket->readAll());
        // 缓冲区上限检查:超过 1MB 返回 413,防止慢速 DoS
        if (m_buffers[socket].size() > 1024 * 1024) {
            m_buffers.erase(socket);
            tooLarge = true;
        }
    }
    // 413 响应在锁外发送,避免持锁执行 socket I/O 阻塞其他连接
    if (tooLarge) {
        HttpResponse resp;
        resp.setStatus(413);
        resp.setBody(QByteArray("Request Entity Too Large"));
        sendResponse(socket, resp);
        return;
    }
    QByteArray data;
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        data = m_buffers[socket];
    }

    HttpRequest req;
    const auto status = parseRequest(socket, req, data);
    if (status == ParseStatus::Incomplete) {
        // 请求不完整,等待后续数据到达,不发送响应
        return;
    }
    if (status == ParseStatus::BadRequest) {
        // 请求格式错误,返回 400 并清理缓冲
        {
            std::lock_guard<std::mutex> lock(m_bufferMutex);
            m_buffers.erase(socket);
        }
        HttpResponse resp;
        resp.setStatus(400);
        resp.setBody(QByteArray("Bad Request"));
        sendResponse(socket, resp);
        return;
    }

    // 解析成功,清理缓冲
    {
        std::lock_guard<std::mutex> lock(m_bufferMutex);
        m_buffers.erase(socket);
    }

    // 计时开始
    auto startTime = std::chrono::steady_clock::now();

    HttpResponse response;

    // === 中间件 Before 链(按注册顺序) ===
    const auto& middlewares = m_middlewareChain->middlewares();
    bool continueChain = true;
    for (const auto& mw : middlewares) {
        try {
            if (!mw->before(req, response)) {
                continueChain = false;
                break;  // 短路:不再执行后续中间件和 Handler
            }
        } catch (const std::exception& e) {
            SC_ERROR("HttpServer: middleware " + mw->name() +
                     " before() exception: " + std::string(e.what()));
            response.setStatus(500);
            response.setBody(QByteArray("Internal Server Error"));
            continueChain = false;
            break;
        } catch (...) { // Blanket catch: prevent middleware exception from crashing server
            SC_ERROR("HttpServer: middleware " + mw->name() +
                     " before() unknown exception");
            response.setStatus(500);
            response.setBody(QByteArray("Internal Server Error"));
            continueChain = false;
            break;
        }
    }

    // === 路由 Handler(仅在 Before 链全部通过时执行) ===
    if (continueChain) {
        // 查找路由 (v2.7.0: 支持路径参数)
        QMap<QString, QString> pathParams;
        RouteHandler handler = findRoute(req.method(), req.path(), &pathParams);

        if (!pathParams.isEmpty()) {
            req.setPathParams(std::move(pathParams));
        }

        // 读取 404 处理器时加锁拷贝,避免与 setNotFoundHandler 并发数据竞争
        RouteHandler notFoundHandler;
        {
            std::lock_guard<std::mutex> lock(m_routeMutex);
            notFoundHandler = m_notFoundHandler;
        }

        if (handler) {
            try {
                handler(req, response);
            } catch (const std::exception& e) {
                SC_ERROR("HttpServer: handler exception: " + std::string(e.what()));
                response.setStatus(500);
                response.setBody(QByteArray("Internal Server Error"));
            } catch (...) {
                // Blanket catch: HTTP handler 必须捕获所有异常,避免崩溃 server 进程。
                SC_ERROR("HttpServer: handler unknown exception");
                response.setStatus(500);
                response.setBody(QByteArray("Internal Server Error"));
            }
        } else {
            // 404
            if (notFoundHandler) {
                notFoundHandler(req, response);
            } else {
                response.setStatus(404);
                response.setBody(QByteArray("Not Found"));
            }
        }
    }

    // === 计算耗时 ===
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime);

    // === 中间件 After 链(按注册逆序,始终执行) ===
    for (auto it = middlewares.rbegin(); it != middlewares.rend(); ++it) {
        try {
            (*it)->after(req, response, duration);
        } catch (const std::exception& e) {
            SC_ERROR("HttpServer: middleware " + (*it)->name() +
                     " after() exception: " + std::string(e.what()));
        } catch (...) { // Blanket catch: prevent middleware after() exception from crashing server
            SC_ERROR("HttpServer: middleware " + (*it)->name() +
                     " after() unknown exception");
        }
    }

    sendResponse(socket, response);
    emit requestHandled(QString::fromUtf8(toString(req.method())), req.path(), response.status());
}

void HttpServer::onDisconnected() {
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    if (socket) {
        {
            std::lock_guard<std::mutex> lock(m_bufferMutex);
            m_buffers.erase(socket);
        }
        m_currentConnections.fetch_sub(1);
        socket->deleteLater();
    }
}

HttpServer::ParseStatus HttpServer::parseRequest(QTcpSocket* socket, HttpRequest& req, const QByteArray& data) {
    // 找到 header/body 分隔符 \r\n\r\n
    int headerEnd = data.indexOf("\r\n\r\n");
    if (headerEnd < 0) {
        return ParseStatus::Incomplete;  // 请求不完整
    }

    QByteArray headerPart = data.left(headerEnd);
    QByteArray bodyPart = data.mid(headerEnd + 4);

    // 解析请求行
    int firstLineEnd = headerPart.indexOf("\r\n");
    if (firstLineEnd < 0) {
        return ParseStatus::Incomplete;
    }
    QByteArray requestLine = headerPart.left(firstLineEnd);
    QList<QByteArray> parts = requestLine.split(' ');
    if (parts.size() < 3) {
        return ParseStatus::BadRequest;
    }

    // 方法
    auto methodOpt = fromString(parts[0].toStdString());
    if (!methodOpt) {
        return ParseStatus::BadRequest;
    }
    req.setMethod(*methodOpt);

    // URI + 解析 path 和 query
    QString uri = QString::fromUtf8(parts[1]);
    req.setUri(uri);
    QUrl url(uri);
    req.setPath(url.path());
    QUrlQuery query(url);
    QMap<QString, QString> queryParams;
    const auto queryItems = query.queryItems();
    for (const auto& item : queryItems) {
        queryParams.insert(item.first, item.second);
    }
    req.setQueryParams(queryParams);

    // 解析 header(逐行解析,\r\n 分隔)
    QByteArray headerLines = headerPart.mid(firstLineEnd + 2);
    int pos = 0;
    while (pos < headerLines.size()) {
        int lineEnd = headerLines.indexOf("\r\n", pos);
        if (lineEnd < 0) {
            lineEnd = headerLines.size();
        }
        QByteArray line = headerLines.mid(pos, lineEnd - pos);
        pos = lineEnd + 2;
        if (line.isEmpty()) continue;
        int colon = line.indexOf(':');
        if (colon > 0) {
            QString key = QString::fromUtf8(line.left(colon)).trimmed();
            QString value = QString::fromUtf8(line.mid(colon + 1)).trimmed();
            req.setHeader(key, value);
        }
    }

    // body(根据 Content-Length 判断是否完整)
    // HTTP/1.1 规范: 无 Content-Length 且无 chunked 编码时,请求体长度为 0
    // 不能将 header 后剩余数据当作 body,否则 pipelining 场景下会吞掉后续请求
    QString contentLengthStr = req.header("Content-Length");
    if (!contentLengthStr.isEmpty()) {
        int expected = contentLengthStr.toInt();
        if (bodyPart.size() < expected) {
            return ParseStatus::Incomplete;  // body 不完整
        }
        req.setBody(bodyPart.left(expected));
    } else {
        req.setBody(QByteArray());  // 无 Content-Length 时 body 为空(符合 HTTP/1.1 规范)
    }

    req.setPeerAddress(socket->peerAddress());
    return ParseStatus::Ok;
}

RouteHandler HttpServer::findRoute(HttpMethod method, const QString& path,
                                   QMap<QString, QString>* pathParams) const {
    std::lock_guard<std::mutex> lock(m_routeMutex);

    // 1. 精确匹配优先
    auto it = m_routes.find({method, path});
    if (it != m_routes.end()) {
        return it->second;
    }

    // 2. 模式匹配 (路径参数 :id 和通配符 *path) [v2.7.0]
    auto reqSegments = splitPath(path);
    for (const auto& [key, handler] : m_patternRoutes) {
        if (key.method != method) continue;

        auto patternSegments = splitPath(key.path);
        QMap<QString, QString> params;
        if (matchPattern(patternSegments, reqSegments, params)) {
            if (pathParams) {
                *pathParams = std::move(params);
            }
            return handler;
        }
    }

    return nullptr;
}

std::vector<QString> HttpServer::splitPath(const QString& path) {
    std::vector<QString> segments;
    if (path.isEmpty() || path == "/") {
        return segments;
    }

    QString p = path;
    if (p.startsWith('/')) p = p.mid(1);

    const auto parts = p.split('/');
    for (const auto& part : parts) {
        if (!part.isEmpty()) {
            segments.push_back(part);
        }
    }
    return segments;
}

bool HttpServer::matchPattern(const std::vector<QString>& pattern,
                               const std::vector<QString>& request,
                               QMap<QString, QString>& pathParams) {
    size_t pi = 0, ri = 0;

    while (pi < pattern.size() && ri < request.size()) {
        const auto& pseg = pattern[pi];
        const auto& rseg = request[ri];

        if (pseg.startsWith('*')) {
            // 通配符: 匹配剩余所有路径
            QString wildcard = rseg;
            for (size_t j = ri + 1; j < request.size(); ++j) {
                wildcard += "/" + request[j];
            }
            pathParams[pseg.mid(1)] = wildcard;
            return true;
        }

        if (pseg.startsWith(':')) {
            // 路径参数
            pathParams[pseg.mid(1)] = rseg;
            ++pi;
            ++ri;
            continue;
        }

        if (pseg != rseg) {
            return false;
        }

        ++pi;
        ++ri;
    }

    // 精确匹配完成 (不含通配符)
    return pi == pattern.size() && ri == request.size();
}

void HttpServer::sendResponse(QTcpSocket* socket, const HttpResponse& response) {
    if (!socket || socket->state() == QAbstractSocket::UnconnectedState) {
        return;
    }
    QByteArray data = response.serialize();
    socket->write(data);
    socket->disconnectFromHost();
}

} // namespace server
} // namespace sc
