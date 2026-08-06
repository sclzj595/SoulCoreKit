#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include "soul/configuration/config_center_client.h"

using namespace sc;

// ============================================================================
// TestConfigCenterClient — 配置中心客户端单元测试
// ============================================================================
class TestConfigCenterClient : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingletonInstance();
    void testInitializeShutdown();
    void testConnectDisconnect();
    void testIsConnected();
    void testGetSetString();
    void testGetSetInt();
    void testGetSetBool();
    void testWatchUnwatch();
    void testGetConfig();
    void testBackend();
    void testConfig();
};

void TestConfigCenterClient::initTestCase() {
}

void TestConfigCenterClient::cleanupTestCase() {
    ConfigCenterClient::instance().shutdown();
}

void TestConfigCenterClient::testSingletonInstance() {
    ConfigCenterClient& client1 = ConfigCenterClient::instance();
    ConfigCenterClient& client2 = ConfigCenterClient::instance();
    QCOMPARE(&client1, &client2);
}

void TestConfigCenterClient::testInitializeShutdown() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    config.endpoints = "http://127.0.0.1:2379";

    auto r = client.initialize(config);
    // Local 后端初始化应该成功
    QVERIFY(r.isOk());
}

void TestConfigCenterClient::testConnectDisconnect() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);

    auto r = client.connect();
    QVERIFY(r.isOk());
    QVERIFY(client.isConnected());

    client.disconnect();
    QVERIFY(!client.isConnected());
}

void TestConfigCenterClient::testIsConnected() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    QVERIFY(!client.isConnected());

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);
    (void)client.connect();

    QVERIFY(client.isConnected());
}

void TestConfigCenterClient::testGetSetString() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);
    (void)client.connect();

    sc::json::Json value = sc::json::Json("hello world");
    auto setResult = client.setConfig("test.string.key", value);
    QVERIFY(setResult.isOk());

    auto getResult = client.getConfig("test.string.key");
    QVERIFY(getResult.isOk());
}

void TestConfigCenterClient::testGetSetInt() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);
    (void)client.connect();

    sc::json::Json value = sc::json::Json(42);
    auto setResult = client.setConfig("test.int.key", value);
    QVERIFY(setResult.isOk());

    auto getResult = client.getConfig("test.int.key");
    QVERIFY(getResult.isOk());
}

void TestConfigCenterClient::testGetSetBool() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);
    (void)client.connect();

    sc::json::Json value = sc::json::Json(true);
    auto setResult = client.setConfig("test.bool.key", value);
    QVERIFY(setResult.isOk());

    auto getResult = client.getConfig("test.bool.key");
    QVERIFY(getResult.isOk());
}

void TestConfigCenterClient::testWatchUnwatch() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);
    (void)client.connect();

    int changeCount = 0;
    auto watchResult = client.watch("watch.key", [&](const ConfigChangeEvent&) {
        changeCount++;
    });
    QVERIFY(watchResult.isOk());

    auto unwatchResult = client.unwatch("watch.key");
    QVERIFY(unwatchResult.isOk());
}

void TestConfigCenterClient::testGetConfig() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);
    (void)client.connect();

    // 测试带默认值的 getConfig
    sc::json::Json defaultValue = sc::json::Json("default");
    auto result = client.getConfig("nonexistent.key", defaultValue);
    QVERIFY(result.isOk());
}

void TestConfigCenterClient::testBackend() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Local;
    (void)client.initialize(config);

    QCOMPARE(client.backend(), ConfigCenterBackend::Local);
}

void TestConfigCenterClient::testConfig() {
    auto& client = ConfigCenterClient::instance();
    client.shutdown();

    ConfigCenterConfig config;
    config.backend = ConfigCenterBackend::Nacos;
    config.endpoints = "http://127.0.0.1:8848";
    config.namespace_ = "public";
    config.group = "DEFAULT_GROUP";
    config.timeoutMs = 5000;
    (void)client.initialize(config);

    QCOMPARE(client.backend(), ConfigCenterBackend::Nacos);
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestConfigCenterClient test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_config_center_client.moc"