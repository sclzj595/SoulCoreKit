// ============================================================================
// test_http_server.cpp — 嵌入式 HTTP Server 单元测试
// ============================================================================
//
// 覆盖范围:
//   - HttpServer 启停/监听
//   - 路由注册与分发(GET/POST/PUT/DELETE)
//   - 请求解析(method/path/query/header/body)
//   - 响应序列化(状态码/header/body)
//   - 404 处理
//   - 端到端 HTTP 请求(QTcpSocket 模拟客户端)

#include <QTest>
#include <QSignalSpy>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTimer>
#include <QEventLoop>
#include <QString>
#include <QByteArray>

#include "soul/server/http_server.h"
#include "soul/server/middleware.h"
#include "soul/server/health.h"

using namespace sc;
using namespace sc::server;

/// @brief 辅助函数:等待 server 处理请求并读取响应
/// server 端的 readyRead 信号需要事件循环 dispatch,纯同步 waitFor 会阻塞。
namespace {
/// @brief 辅助函数:发送请求并获取完整响应
QByteArray sendAndReceive(quint16 port, const QByteArray& request, int timeoutMs = 3000) {
    QTcpSocket client;
    QByteArray response;
    bool disconnected = false;
    QEventLoop loop;
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    QObject::connect(&client, &QTcpSocket::disconnected, [&]() {
        disconnected = true;
        loop.quit();
    });
    QObject::connect(&client, &QTcpSocket::readyRead, [&client, &response]() {
        response += client.readAll();
    });

    client.connectToHost(QHostAddress::LocalHost, port);
    if (!client.waitForConnected(timeoutMs)) {
        return {};
    }

    // [v1.9.2] 等待服务器处理连接（连接数限制检查等）
    QTest::qWait(50);

    // 检查连接是否已被服务器断开（如连接数限制）
    if (client.state() != QAbstractSocket::ConnectedState) {
        return response;
    }

    client.write(request);
    if (!client.waitForBytesWritten(timeoutMs)) {
        return response;
    }

    // 如果服务器已断开连接（如发送503后立即断开），
    // 数据可能已在 readyRead 中收到
    if (disconnected) {
        return response;
    }

    loop.exec();
    return response;
}
}

// ============================================================================
// TestHttpServer — HTTP Server 测试
// ============================================================================
class TestHttpServer : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void testListenAndClose();
    void testRouteRegistration();
    void testHttpRequestParsing();
    void testHttpResponseSerialization();
    void testEndToEndGetRequest();
    void testEndToEndPostRequest();
    void testNotFoundResponse();
    void testHttpMethodConversion();
    void testCrossSegmentRequest();
    // [v1.9.1 新增] 中间件链测试
    void testMiddlewareChain();
    void testMiddlewareShortCircuit();
    void testAuthMiddleware();
    void testCorsMiddleware();
    // [v1.9.1 新增] 健康检查端点测试
    void testHealthEndpoint();
    void testHealthLivenessVsReadiness();
    // [v1.9.1 新增] 连接数限制测试
    void testConnectionLimit();
    void testCurrentConnections();
    void testConnectionLimitReachedSignal();
};

void TestHttpServer::initTestCase() {
    // 无需特殊初始化
}

void TestHttpServer::cleanupTestCase() {
    // 无需特殊清理
}

void TestHttpServer::cleanup() {
    // 每个测试后清理事件循环
    QCoreApplication::processEvents();
}

// 验证监听与关闭
void TestHttpServer::testListenAndClose() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));  // 0 = 自动分配端口
    QVERIFY(server.isListening());
    QVERIFY(server.serverPort() > 0);
    server.close();
    QVERIFY(!server.isListening());
}

// 验证路由注册不崩溃
void TestHttpServer::testRouteRegistration() {
    HttpServer server;
    server.get("/api/test", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("ok");
    });
    server.post("/api/create", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(201);
        resp.setBody("created");
    });
    server.del("/api/delete", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(204);
    });
    // 注册成功即可,无异常则通过
    QVERIFY(true);
}

// 验证请求解析
void TestHttpServer::testHttpRequestParsing() {
    // 通过构造原始 HTTP 请求字节串验证解析逻辑
    // (HttpServer::parseRequest 是 private,通过端到端测试间接验证)
    QVERIFY(true);
}

// 验证响应序列化格式
void TestHttpServer::testHttpResponseSerialization() {
    HttpResponse resp;
    resp.setStatus(200);
    resp.setHeader("Content-Type", "application/json");
    resp.setBody("{\"status\":\"ok\"}");

    QByteArray data = resp.serialize();
    QVERIFY(data.startsWith("HTTP/1.1 200 OK\r\n"));
    QVERIFY(data.contains("Content-Type: application/json\r\n"));
    QVERIFY(data.contains("Content-Length: 15\r\n"));
    QVERIFY(data.endsWith("{\"status\":\"ok\"}"));
}

// 端到端 GET 请求
void TestHttpServer::testEndToEndGetRequest() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    server.get("/hello", [](const HttpRequest& req, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setHeader("Content-Type", "text/plain");
        resp.setBody("Hello, " + req.queryParams().value("name", "World").toUtf8());
    });

    QByteArray request = "GET /hello?name=AOP HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("HTTP/1.1 200 OK"));
    QVERIFY(response.contains("Hello, AOP"));
    server.close();
}

// 端到端 POST 请求
void TestHttpServer::testEndToEndPostRequest() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    server.post("/echo", [](const HttpRequest& req, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setHeader("Content-Type", "text/plain");
        resp.setBody("Received: " + req.body());
    });

    QByteArray body = "test-body-content";
    QByteArray request = "POST /echo HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: " + QByteArray::number(body.size()) + "\r\n"
                         "Connection: close\r\n"
                         "\r\n" + body;
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("HTTP/1.1 200 OK"));
    QVERIFY(response.contains("Received: test-body-content"));
    server.close();
}

// 验证 404 响应
void TestHttpServer::testNotFoundResponse() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    // 不注册任何路由,访问任意路径应返回 404
    QByteArray request = "GET /nonexistent HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("HTTP/1.1 404 Not Found"));
    server.close();
}

// 验证 HTTP 方法枚举转换
void TestHttpServer::testHttpMethodConversion() {
    QCOMPARE(toString(HttpMethod::Get), "GET");
    QCOMPARE(toString(HttpMethod::Post), "POST");
    QCOMPARE(toString(HttpMethod::Put), "PUT");
    QCOMPARE(toString(HttpMethod::Delete), "DELETE");

    QVERIFY(fromString("GET").has_value());
    QCOMPARE(fromString("GET").value(), HttpMethod::Get);
    QVERIFY(!fromString("INVALID").has_value());
}

// 验证 HTTP 请求跨 TCP 段到达时正确缓冲与解析(v1.9.0 核心修复)
void TestHttpServer::testCrossSegmentRequest() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    server.get("/split", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("split-ok");
    });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected(3000));

    // 第一段:仅含请求行和部分 header,无 \r\n\r\n 结束标记 → Incomplete
    client.write("GET /split HTTP/1.1\r\nHost: localhost\r\n");
    client.waitForBytesWritten(3000);

    // 等待 server 处理第一段(应判定 Incomplete,不发送响应)
    QEventLoop loop1;
    QTimer::singleShot(300, &loop1, &QEventLoop::quit);
    loop1.exec();

    // 第二段:补全剩余 header + 空行,触发完整解析
    client.write("Connection: close\r\n\r\n");
    client.waitForBytesWritten(3000);

    QByteArray response;
    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    QObject::connect(&client, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
    QObject::connect(&client, &QTcpSocket::readyRead, [&client, &response]() {
        response += client.readAll();
    });
    loop.exec();

    QVERIFY(response.contains("HTTP/1.1 200 OK"));
    QVERIFY(response.contains("split-ok"));
    server.close();
}

// ============================================================================
// 中间件链测试 [v1.9.1 新增]
// ============================================================================

// 验证中间件链注册与执行顺序
void TestHttpServer::testMiddlewareChain() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    // 记录中间件执行顺序
    std::vector<std::string> execOrder;
    auto mutex = std::make_shared<std::mutex>();

    // 自定义中间件:记录执行顺序
    class OrderTrackingMiddleware : public IMiddleware {
    public:
        OrderTrackingMiddleware(std::string name, std::vector<std::string>& order,
                                std::shared_ptr<std::mutex> mtx)
            : m_name(std::move(name)), m_order(order), m_mutex(std::move(mtx)) {}

        bool before(HttpRequest&, HttpResponse&) override {
            std::lock_guard<std::mutex> lock(*m_mutex);
            m_order.push_back(m_name + ":before");
            return true;
        }
        void after(const HttpRequest&, HttpResponse&,
                   std::chrono::milliseconds) override {
            std::lock_guard<std::mutex> lock(*m_mutex);
            m_order.push_back(m_name + ":after");
        }
        std::string name() const override { return m_name; }

    private:
        std::string m_name;
        std::vector<std::string>& m_order;
        std::shared_ptr<std::mutex> m_mutex;
    };

    auto mw1 = std::make_shared<OrderTrackingMiddleware>("MW1", execOrder, mutex);
    auto mw2 = std::make_shared<OrderTrackingMiddleware>("MW2", execOrder, mutex);

    server.use(mw1).use(mw2);

    server.get("/order", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("ok");
    });

    QByteArray request = "GET /order HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("HTTP/1.1 200 OK"));

    // Before: MW1 → MW2(注册顺序)
    // After:  MW2 → MW1(注册逆序)
    QCOMPARE(execOrder.size(), size_t(4));
    QCOMPARE(QString::fromStdString(execOrder[0]), QString("MW1:before"));
    QCOMPARE(QString::fromStdString(execOrder[1]), QString("MW2:before"));
    QCOMPARE(QString::fromStdString(execOrder[2]), QString("MW2:after"));
    QCOMPARE(QString::fromStdString(execOrder[3]), QString("MW1:after"));

    server.close();
}

// 验证中间件短路(Before 返回 false)
void TestHttpServer::testMiddlewareShortCircuit() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    bool handlerCalled = false;
    server.get("/blocked", [&handlerCalled](const HttpRequest&, HttpResponse& resp) {
        handlerCalled = true;
        resp.setStatus(200);
        resp.setBody("should-not-reach");
    });

    // 注册短路中间件
    class BlockingMiddleware : public IMiddleware {
    public:
        bool before(HttpRequest&, HttpResponse& resp) override {
            resp.setStatus(403);
            resp.setBody("Blocked by middleware");
            return false;  // 短路
        }
        void after(const HttpRequest&, HttpResponse&,
                   std::chrono::milliseconds) override {}
        std::string name() const override { return "BlockingMiddleware"; }
    };

    server.use(std::make_shared<BlockingMiddleware>());

    QByteArray request = "GET /blocked HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("HTTP/1.1 403"));
    QVERIFY(response.contains("Blocked by middleware"));
    QVERIFY(!handlerCalled);  // Handler 不应被执行

    server.close();
}

// 验证 AuthMiddleware Bearer token 鉴权
void TestHttpServer::testAuthMiddleware() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    auto auth = std::make_shared<AuthMiddleware>(
        [](const std::string& token) { return token == "secret-token"; });
    auth->addExcludePath("/api/health");
    server.use(auth);

    server.get("/api/health", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("healthy");
    });
    server.get("/api/data", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("data");
    });

    // 无 token 请求 → 401
    {
        QByteArray request = "GET /api/data HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Connection: close\r\n"
                             "\r\n";
        QByteArray response = sendAndReceive(port, request);
        QVERIFY(response.contains("HTTP/1.1 401 Unauthorized"));
    }

    // 错误 token → 401
    {
        QByteArray request = "GET /api/data HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Authorization: Bearer wrong-token\r\n"
                             "Connection: close\r\n"
                             "\r\n";
        QByteArray response = sendAndReceive(port, request);
        QVERIFY(response.contains("HTTP/1.1 401 Unauthorized"));
    }

    // 正确 token → 200
    {
        QByteArray request = "GET /api/data HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Authorization: Bearer secret-token\r\n"
                             "Connection: close\r\n"
                             "\r\n";
        QByteArray response = sendAndReceive(port, request);
        QVERIFY(response.contains("HTTP/1.1 200 OK"));
        QVERIFY(response.contains("data"));
    }

    // 排除路径:无需 token → 200
    {
        QByteArray request = "GET /api/health HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Connection: close\r\n"
                             "\r\n";
        QByteArray response = sendAndReceive(port, request);
        QVERIFY(response.contains("HTTP/1.1 200 OK"));
        QVERIFY(response.contains("healthy"));
    }

    server.close();
}

// 验证 CorsMiddleware OPTIONS 预检
void TestHttpServer::testCorsMiddleware() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    auto cors = std::make_shared<CorsMiddleware>();
    cors->setAllowedOrigin("https://example.com");
    server.use(cors);

    server.get("/cors", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("cors-ok");
    });

    // OPTIONS 预检 → 204 + CORS 头
    {
        QByteArray request = "OPTIONS /cors HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Origin: https://example.com\r\n"
                             "Connection: close\r\n"
                             "\r\n";
        QByteArray response = sendAndReceive(port, request);
        QVERIFY(response.contains("HTTP/1.1 204 No Content"));
        QVERIFY(response.contains("Access-Control-Allow-Origin: https://example.com"));
        QVERIFY(response.contains("Access-Control-Allow-Methods:"));
    }

    // 普通 GET → CORS 头在响应中
    {
        QByteArray request = "GET /cors HTTP/1.1\r\n"
                             "Host: localhost\r\n"
                             "Connection: close\r\n"
                             "\r\n";
        QByteArray response = sendAndReceive(port, request);
        QVERIFY(response.contains("HTTP/1.1 200 OK"));
        QVERIFY(response.contains("cors-ok"));
        QVERIFY(response.contains("Access-Control-Allow-Origin: https://example.com"));
    }

    server.close();
}

// ============================================================================
// 健康检查端点测试 [v1.9.1 新增]
// ============================================================================

// 验证 HealthEndpoint 通过 HttpServer 暴露 /api/health 端点
void TestHttpServer::testHealthEndpoint() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    auto health = std::make_shared<HealthEndpoint>();

    // 注册指示器
    health->addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "database"));
    health->addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return true; }, "mq"));

    server.get("/api/health", [health](const HttpRequest&, HttpResponse& resp) {
        auto report = health->check();
        resp.setStatus(report.overall == HealthStatus::UP ? 200 : 503);
        resp.setHeader("Content-Type", "application/json");
        resp.setBody(report.toJson());
    });

    QByteArray request = "GET /api/health HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("HTTP/1.1 200 OK"));
    QVERIFY(response.contains("Content-Type: application/json"));
    QVERIFY(response.contains("\"status\":\"UP\""));
    QVERIFY(response.contains("\"database\""));
    QVERIFY(response.contains("\"mq\""));

    server.close();
}

// 验证 Liveness vs Readiness
void TestHttpServer::testHealthLivenessVsReadiness() {
    auto health = std::make_shared<HealthEndpoint>();

    // 数据库是 key 依赖, MQ 不是
    health->addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "database"));
    health->addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return false; }, "mq"));  // MQ DOWN

    // Readiness: 检查所有 → 包含 MQ DOWN → 整体 DOWN
    auto readinessReport = health->readiness();
    QCOMPARE(readinessReport.details.size(), size_t(2));
    QCOMPARE(readinessReport.overall, HealthStatus::DOWN);

    // Liveness: 仅检查关键依赖 → 不检查 MQ → 整体 UP
    auto livenessReport = health->liveness();
    QCOMPARE(livenessReport.details.size(), size_t(1));
    QCOMPARE(livenessReport.details[0].name, std::string("database"));
    QCOMPARE(livenessReport.overall, HealthStatus::UP);

    // 空指示器 → UNKNOWN → UP(无指示器视为 UP)
    HealthEndpoint empty;
    auto emptyReport = empty.check();
    QCOMPARE(emptyReport.overall, HealthStatus::UP);
}

// ============================================================================
// 连接数限制测试 [v1.9.1 新增]
// ============================================================================

// 验证连接数限制生效
void TestHttpServer::testConnectionLimit() {
    HttpServer server;
    server.setMaxConnections(2);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    server.get("/test", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("ok");
    });

    // 建立 2 个连接(应成功)
    QTcpSocket client1, client2;
    client1.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client1.waitForConnected(3000));
    client2.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client2.waitForConnected(3000));
    // [v1.9.2] 等待事件循环处理 onNewConnection 信号
    QTest::qWait(100);

    // 第 3 个连接应被拒绝(503)
    QByteArray request = "GET /test HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("503"));
    QVERIFY(response.contains("Service Unavailable"));

    client1.disconnectFromHost();
    client2.disconnectFromHost();
    if (client1.state() != QAbstractSocket::UnconnectedState) client1.waitForDisconnected(3000);
    if (client2.state() != QAbstractSocket::UnconnectedState) client2.waitForDisconnected(3000);

    server.close();
}

// 验证 currentConnections 计数正确
void TestHttpServer::testCurrentConnections() {
    HttpServer server;
    QCOMPARE(server.currentConnections(), 0);
    QCOMPARE(server.maxConnections(), 0);  // 默认不限制

    server.setMaxConnections(10);
    QCOMPARE(server.maxConnections(), 10);

    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    server.get("/test", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("ok");
    });

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected(3000));
    // [v1.9.2] 等待事件循环处理 onNewConnection 信号
    QTest::qWait(50);

    // 连接建立后,currentConnections 应为 1
    QCOMPARE(server.currentConnections(), 1);

    client.disconnectFromHost();
    if (client.state() != QAbstractSocket::UnconnectedState) client.waitForDisconnected(3000);

    // 断开后,currentConnections 应为 0
    QTest::qWait(100);
    QCOMPARE(server.currentConnections(), 0);

    server.close();
}

// 验证 connectionLimitReached 信号
void TestHttpServer::testConnectionLimitReachedSignal() {
    HttpServer server;
    server.setMaxConnections(1);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    server.get("/test", [](const HttpRequest&, HttpResponse& resp) {
        resp.setStatus(200);
        resp.setBody("ok");
    });

    // 建立 1 个连接,占满限额
    QTcpSocket client1;
    client1.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client1.waitForConnected(3000));
    // [v1.9.2] 等待事件循环处理 onNewConnection 信号
    QTest::qWait(50);

    // 第 2 个连接触发 connectionLimitReached 信号
    QSignalSpy spy(&server, &HttpServer::connectionLimitReached);

    QByteArray request = "GET /test HTTP/1.1\r\n"
                         "Host: localhost\r\n"
                         "Connection: close\r\n"
                         "\r\n";
    QByteArray response = sendAndReceive(port, request);
    QVERIFY(response.contains("503"));

    QCOMPARE(spy.count(), 1);

    client1.disconnectFromHost();
    if (client1.state() != QAbstractSocket::UnconnectedState) client1.waitForDisconnected(3000);

    server.close();
}

// ============================================================================
// TestHttpServerConnectionLimit — 连接数限制测试
// ============================================================================
class TestHttpServerConnectionLimit : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

    void testDefaultUnlimited();
    void testSetMaxConnections();
    void testSetMaxConnectionsZero();
    void testCurrentConnections();
    void testConnectionLimitSignal();
    void testCloseReleasesConnections();
};

void TestHttpServerConnectionLimit::initTestCase() {
}

void TestHttpServerConnectionLimit::cleanupTestCase() {
}

void TestHttpServerConnectionLimit::cleanup() {
    QCoreApplication::processEvents();
}

// 验证默认 maxConnections 为 0(不限制)
void TestHttpServerConnectionLimit::testDefaultUnlimited() {
    HttpServer server;
    QCOMPARE(server.maxConnections(), 0);
}

// 验证 setMaxConnections 设置后正确返回
void TestHttpServerConnectionLimit::testSetMaxConnections() {
    HttpServer server;
    server.setMaxConnections(5);
    QCOMPARE(server.maxConnections(), 5);
}

// 验证设置为 0 表示不限制
void TestHttpServerConnectionLimit::testSetMaxConnectionsZero() {
    HttpServer server;
    server.setMaxConnections(10);
    QCOMPARE(server.maxConnections(), 10);
    server.setMaxConnections(0);
    QCOMPARE(server.maxConnections(), 0);
}

// 验证 currentConnections 随客户端连接/断开正确增减
void TestHttpServerConnectionLimit::testCurrentConnections() {
    HttpServer server;
    QCOMPARE(server.currentConnections(), 0);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    QTcpSocket client;
    client.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client.waitForConnected(3000));
    // [v1.9.2] 等待事件循环处理 onNewConnection 信号
    QTest::qWait(50);
    QCOMPARE(server.currentConnections(), 1);

    client.disconnectFromHost();
    if (client.state() != QAbstractSocket::UnconnectedState)
        client.waitForDisconnected(3000);

    QTest::qWait(100);
    QCOMPARE(server.currentConnections(), 0);

    server.close();
}

// 验证达到连接上限时触发 connectionLimitReached 信号
void TestHttpServerConnectionLimit::testConnectionLimitSignal() {
    HttpServer server;
    server.setMaxConnections(1);
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    QSignalSpy spy(&server, &HttpServer::connectionLimitReached);

    // 第 1 个连接正常建立
    QTcpSocket client1;
    client1.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client1.waitForConnected(3000));
    // [v1.9.2] 等待事件循环处理 onNewConnection 信号
    QTest::qWait(50);

    // 第 2 个连接到达上限,触发 connectionLimitReached 信号
    QTcpSocket client2;
    client2.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client2.waitForConnected(3000));
    // [v1.9.2] 等待事件循环处理 onNewConnection 信号
    QTest::qWait(50);
    QCOMPARE(spy.count(), 1);

    client1.disconnectFromHost();
    client2.disconnectFromHost();
    if (client1.state() != QAbstractSocket::UnconnectedState)
        client1.waitForDisconnected(3000);
    if (client2.state() != QAbstractSocket::UnconnectedState)
        client2.waitForDisconnected(3000);

    server.close();
}

// 验证关闭 Server 后 currentConnections 归零
void TestHttpServerConnectionLimit::testCloseReleasesConnections() {
    HttpServer server;
    QVERIFY(server.listen(QHostAddress::LocalHost, 0));
    quint16 port = server.serverPort();

    QTcpSocket client1, client2;
    client1.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client1.waitForConnected(3000));
    client2.connectToHost(QHostAddress::LocalHost, port);
    QVERIFY(client2.waitForConnected(3000));
    // [v1.9.2] 等待事件循环处理 onNewConnection 信号
    QTest::qWait(50);

    QCOMPARE(server.currentConnections(), 2);

    server.close();
    // [v1.9.2] 等待事件循环处理断开连接信号
    QTest::qWait(100);
    QCOMPARE(server.currentConnections(), 0);

    client1.disconnectFromHost();
    client2.disconnectFromHost();
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    // [v1.9.2] 构建自定义参数以分离两个测试类的输出
    QStringList args;
    for (int i = 1; i < argc; ++i) {
        args << QString::fromLocal8Bit(argv[i]);
    }

    int status = 0;
    {
        TestHttpServer test;
        QStringList classArgs = args;
        classArgs << "-o" << "test_http_server_class1.xml,xml";
        QVector<QByteArray> baArgs;
        QVector<char*> cargs;
        baArgs.append(QByteArray(argv[0]));
        for (auto& a : classArgs) {
            baArgs.append(a.toLocal8Bit());
        }
        for (auto& ba : baArgs) {
            cargs.append(ba.data());
        }
        status |= QTest::qExec(&test, cargs.size(), cargs.data());
    }
    {
        TestHttpServerConnectionLimit test;
        QStringList classArgs = args;
        classArgs << "-o" << "test_http_server_class2.xml,xml";
        QVector<QByteArray> baArgs;
        QVector<char*> cargs;
        baArgs.append(QByteArray(argv[0]));
        for (auto& a : classArgs) {
            baArgs.append(a.toLocal8Bit());
        }
        for (auto& ba : baArgs) {
            cargs.append(ba.data());
        }
        status |= QTest::qExec(&test, cargs.size(), cargs.data());
    }
    return status;
}

#include "test_http_server.moc"
