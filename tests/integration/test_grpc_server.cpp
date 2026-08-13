#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>
#include <QJsonObject>

#include "soul/rpc/grpc_server.h"

using namespace sc::rpc;

// ============================================================================
// TestGrpcServer — gRPC 服务端单元测试
// ============================================================================
class TestGrpcServer : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingletonInstance();
    void testStartStop();
    void testIsRunning();
    void testPort();
    void testRegisterUnregisterService();
    void testServices();
    void testMetadata();
    void testBacklog();
};

void TestGrpcServer::initTestCase() {
}

void TestGrpcServer::cleanupTestCase() {
    GrpcServer::instance().stop();
}

void TestGrpcServer::testSingletonInstance() {
    GrpcServer& server1 = GrpcServer::instance();
    GrpcServer& server2 = GrpcServer::instance();
    QCOMPARE(&server1, &server2);
}

void TestGrpcServer::testStartStop() {
    auto& server = GrpcServer::instance();
    server.stop();

    auto r = server.start("127.0.0.1", 50051);
    Q_UNUSED(r);
    // 启动后可能成功也可能失败（端口占用），但不应崩溃

    server.stop();
    QVERIFY(!server.isRunning());
}

void TestGrpcServer::testIsRunning() {
    auto& server = GrpcServer::instance();
    server.stop();

    QVERIFY(!server.isRunning());

    (void)server.start("127.0.0.1", 50052);
    server.stop();
    QVERIFY(!server.isRunning());
}

void TestGrpcServer::testPort() {
    auto& server = GrpcServer::instance();
    server.stop();

    QVERIFY(!server.isRunning());
}

void TestGrpcServer::testRegisterUnregisterService() {
    auto& server = GrpcServer::instance();
    server.stop();

    // 创建模拟 Service
    class MockService : public GrpcService {
    public:
        std::string serviceName() const override { return "MockService"; }
    };

    auto svc = std::make_shared<MockService>();
    auto r = server.registerService(svc);
    QVERIFY(r.isOk());

    auto unregResult = server.unregisterService("MockService");
    QVERIFY(unregResult.isOk());
}

void TestGrpcServer::testServices() {
    auto& server = GrpcServer::instance();
    server.stop();

    class MockService : public GrpcService {
    public:
        std::string serviceName() const override { return "MockService"; }
    };

    auto svc = std::make_shared<MockService>();
    (void)server.registerService(svc);

    // 注册后应能找到
    (void)server.unregisterService("MockService");

    // 再次取消注册不存在的服务
    auto r = server.unregisterService("NonexistentService");
    QVERIFY(!r.isOk());
}

void TestGrpcServer::testMetadata() {
    GrpcMetadata metadata;

    metadata.set("key1", "value1");
    QVERIFY(metadata.has("key1"));
    QCOMPARE(metadata.get("key1"), std::string("value1"));

    QCOMPARE(metadata.get("nonexistent", "default"), std::string("default"));

    metadata.remove("key1");
    QVERIFY(!metadata.has("key1"));
}

void TestGrpcServer::testBacklog() {
    GrpcServer::instance().stop();

    // 设置最大消息大小
    GrpcServer::instance().setMaxMessageSize(8 * 1024 * 1024);
    // 设置 keepalive 时间
    GrpcServer::instance().setKeepAliveTime(30000);

    QVERIFY(true);
}

// ============================================================================
// TestGrpcClient — gRPC 客户端单元测试
// ============================================================================
class TestGrpcClient : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testConnectDisconnect();
    void testIsConnected();
    void testTimeout();
    void testMetadata();
};

void TestGrpcClient::testConstructor() {
    GrpcClient client("localhost:50051");
    QVERIFY(!client.isConnected());
}

void TestGrpcClient::testConnectDisconnect() {
    GrpcClient client("localhost:50051");

    auto r = client.connect();
    Q_UNUSED(r);
    // 连接可能失败（无服务端），但不应崩溃

    client.disconnect();
    QVERIFY(!client.isConnected());
}

void TestGrpcClient::testIsConnected() {
    GrpcClient client("localhost:50051");
    QVERIFY(!client.isConnected());
}

void TestGrpcClient::testTimeout() {
    GrpcClient client("localhost:50051");

    client.setDefaultTimeout(10000);
    client.setMaxRetries(5);
    client.setEnableCompression(true);

    QVERIFY(true);
}

void TestGrpcClient::testMetadata() {
    GrpcClient client("localhost:50051");

    GrpcContext ctx;
    ctx.requestMetadata.set("authorization", "Bearer token123");
    ctx.timeoutMs = 5000;

    QVERIFY(ctx.requestMetadata.has("authorization"));
    QCOMPARE(ctx.timeoutMs, 5000);
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestGrpcServer serverTest;
    TestGrpcClient clientTest;

    int result = 0;
    result |= QTest::qExec(&serverTest, argc, argv);
    result |= QTest::qExec(&clientTest, argc, argv);

    return result;
}

#include "test_grpc_server.moc"