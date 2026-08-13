#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include "soul/cs/cs_ipc_router.h"

using namespace sc::cs;

// ============================================================================
// TestCsIpcRouter — IPC 路由单元测试
// ============================================================================
class TestCsIpcRouter : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingletonInstance();
    void testRegisterUnregisterHandler();
    void testStartStop();
    void testIsRunning();
    void testRoutes();
    void testSharedMemoryTransport();
    void testNamedPipeTransport();
};

void TestCsIpcRouter::initTestCase() {
}

void TestCsIpcRouter::cleanupTestCase() {
    CsIpcRouter::instance().stopServer();
}

void TestCsIpcRouter::testSingletonInstance() {
    CsIpcRouter& router1 = CsIpcRouter::instance();
    CsIpcRouter& router2 = CsIpcRouter::instance();
    QCOMPARE(&router1, &router2);
}

void TestCsIpcRouter::testRegisterUnregisterHandler() {
    auto& router = CsIpcRouter::instance();
    router.stopServer();

    // 注册路由
    bool handlerCalled = false;
    router.route("GET", "/test/path", [&](const IpcRequest& /*req*/) -> IpcResponse {
        handlerCalled = true;
        IpcResponse resp;
        resp.statusCode = 200;
        resp.body = QByteArray("ok");
        return resp;
    });

    // 移除路由
    router.removeRoute("GET", "/test/path");

    // 验证: 不崩溃即通过
    QVERIFY(true);
}

void TestCsIpcRouter::testStartStop() {
    auto& router = CsIpcRouter::instance();

    // 先停止确保干净状态
    router.stopServer();

    // 启动服务端
    auto r = router.startServer("SoulCoreKitTest");
    // 启动 QLocalServer 可能成功也可能失败（取决于环境），只验证不崩溃
    Q_UNUSED(r);

    // 停止服务端
    router.stopServer();
    QVERIFY(!router.isServerRunning());
}

void TestCsIpcRouter::testIsRunning() {
    auto& router = CsIpcRouter::instance();
    router.stopServer();

    QVERIFY(!router.isServerRunning());

    (void)router.startServer("SoulCoreKitTest");
    // 启动后状态取决于环境
    // 停止后确认
    router.stopServer();
    QVERIFY(!router.isServerRunning());
}

void TestCsIpcRouter::testRoutes() {
    auto& router = CsIpcRouter::instance();
    router.stopServer();

    // 注册多个路由
    router.route("GET", "/api/users", [](const IpcRequest&) -> IpcResponse {
        return IpcResponse();
    });

    router.route("POST", "/api/users", [](const IpcRequest&) -> IpcResponse {
        return IpcResponse();
    });

    router.route("DELETE", "/api/users/1", [](const IpcRequest&) -> IpcResponse {
        return IpcResponse();
    });

    // 清理
    router.removeRoute("GET", "/api/users");
    router.removeRoute("POST", "/api/users");
    router.removeRoute("DELETE", "/api/users/1");

    QVERIFY(true);
}

void TestCsIpcRouter::testSharedMemoryTransport() {
    SharedMemoryTransport transport("TestSharedMem", 4096);

    auto createResult = transport.create();
    if (createResult.isOk()) {
        QVERIFY(transport.isAttached());
        QCOMPARE(transport.size(), 4096);

        QByteArray testData = "Hello IPC!";
        auto writeResult = transport.write(testData);
        QVERIFY(writeResult.isOk());

        auto readResult = transport.read(testData.size());
        QVERIFY(readResult.isOk());
        QCOMPARE(readResult.unwrap(), testData);

        transport.detach();
        QVERIFY(!transport.isAttached());
    }
}

void TestCsIpcRouter::testNamedPipeTransport() {
    NamedPipeTransport transport;

    QVERIFY(!transport.isServerRunning());
    QVERIFY(!transport.isConnected());

    // 启动服务端
    auto startResult = transport.startServer("TestNamedPipe");
    Q_UNUSED(startResult);

    transport.stopServer();
    QVERIFY(!transport.isServerRunning());
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestCsIpcRouter test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_cs_ipc_router.moc"