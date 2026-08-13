#include <QTest>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <memory>
#include <functional>

#include "soul/configuration/remote_config.h"
#include "soul/configuration/nacos_source.h"
#include "soul/configuration/etcd_source.h"
#include "soul/utils/json/json_helper.h"

using namespace sc;

// ============================================================================
// MockRemoteConfigSource — 用于测试的模拟远程配置源
// ============================================================================
class MockRemoteConfigSource : public IRemoteConfigSource {
public:
    sc::json::Json mockConfig = sc::json::Json::object();
    bool connected = false;
    bool disconnected = false;
    QString lastFetchedNamespace;
    QString lastPublishedNamespace;
    sc::json::Json lastPublishedConfig = sc::json::Json::object();
    bool failConnect = false;
    bool failFetch = false;
    bool failPublish = false;

    Result<void> connectToServer() override {
        if (failConnect) {
            return Error(ErrorCode::ConnectionRefused, "Mock connect failed");
        }
        connected = true;
        return Ok();
    }

    Result<sc::json::Json> fetchConfig(const QString& namespaceName) override {
        if (failFetch) {
            return Error(ErrorCode::NotFound, "Mock fetch failed");
        }
        lastFetchedNamespace = namespaceName;
        return Result<sc::json::Json>(mockConfig);
    }

    Result<void> watchConfig(const QString& namespaceName,
                             std::function<void(const sc::json::Json&)> callback) override {
        lastFetchedNamespace = namespaceName;
        // 模拟立即触发一次回调
        if (callback) {
            callback(mockConfig);
        }
        return Ok();
    }

    Result<void> publishConfig(const QString& namespaceName,
                               const sc::json::Json& config) override {
        if (failPublish) {
            return Error(ErrorCode::NetworkError, "Mock publish failed");
        }
        lastPublishedNamespace = namespaceName;
        lastPublishedConfig = config;
        return Ok();
    }

    void disconnectFromServer() override {
        disconnected = true;
    }
};

// ============================================================================
// TestRemoteConfig — 远程配置测试
// ============================================================================
class TestRemoteConfig : public QObject {
    Q_OBJECT

private slots:
    // ===== RemoteConfiguration: 基本 CRUD =====
    void testRemoteConfigDefaults();
    void testRemoteConfigSetGetString();
    void testRemoteConfigSetGetInt();
    void testRemoteConfigSetGetDouble();
    void testRemoteConfigSetGetBool();
    void testRemoteConfigContains();
    void testRemoteConfigRemove();

    // ===== RemoteConfiguration: 信号 =====
    void testRemoteConfigChangeSignal();
    void testRemoteConfigSyncedSignal();

    // ===== RemoteConfiguration: 文件加载/保存 =====
    void testRemoteConfigLoadFromFile();
    void testRemoteConfigSaveToFile();
    void testRemoteConfigLoadNonExistentFile();
    void testRemoteConfigLoadInvalidJson();

    // ===== RemoteConfiguration: 远程源集成 =====
    void testRemoteConfigSetSource();
    void testRemoteConfigSync();
    void testRemoteConfigSyncNoSource();
    void testRemoteConfigWatch();
    void testRemoteConfigWatchNoSource();

    // ===== MockRemoteConfigSource =====
    void testMockSourceConnect();
    void testMockSourceFetch();
    void testMockSourcePublish();
    void testMockSourceDisconnect();
    void testMockSourceFailures();

    // ===== NacosConfigSource =====
    void testNacosSourceConstruction();
    void testNacosSourceDefaultValues();

    // ===== EtcdConfigSource =====
    void testEtcdSourceConstruction();
    void testEtcdSourceDefaultValues();

    // ===== IConfiguration 接口兼容性 =====
    void testRemoteConfigAsIConfiguration();
};

// =========================================================================
// RemoteConfiguration: 基本 CRUD
// =========================================================================

void TestRemoteConfig::testRemoteConfigDefaults() {
    RemoteConfiguration config;
    QCOMPARE(config.getString("missing.key", "fallback"), QString("fallback"));
    QCOMPARE(config.getInt("missing.key", 42), 42);
    QCOMPARE(config.getDouble("missing.key", 3.14), 3.14);
    QVERIFY(config.getBool("missing.key", true));
}

void TestRemoteConfig::testRemoteConfigSetGetString() {
    RemoteConfiguration config;
    config.setString("test.key", "hello world");
    QCOMPARE(config.getString("test.key", ""), QString("hello world"));
}

void TestRemoteConfig::testRemoteConfigSetGetInt() {
    RemoteConfiguration config;
    config.setInt("test.port", 8080);
    QCOMPARE(config.getInt("test.port", 0), 8080);
}

void TestRemoteConfig::testRemoteConfigSetGetDouble() {
    RemoteConfiguration config;
    config.setDouble("test.rate", 2.71828);
    QCOMPARE(config.getDouble("test.rate", 0.0), 2.71828);
}

void TestRemoteConfig::testRemoteConfigSetGetBool() {
    RemoteConfiguration config;
    config.setBool("test.enabled", true);
    QVERIFY(config.getBool("test.enabled", false));

    config.setBool("test.enabled", false);
    QVERIFY(!config.getBool("test.enabled", true));
}

void TestRemoteConfig::testRemoteConfigContains() {
    RemoteConfiguration config;
    QVERIFY(!config.contains("test.key"));
    config.setString("test.key", "value");
    QVERIFY(config.contains("test.key"));
}

void TestRemoteConfig::testRemoteConfigRemove() {
    RemoteConfiguration config;
    config.setString("test.key", "value");
    QVERIFY(config.contains("test.key"));
    config.remove("test.key");
    QVERIFY(!config.contains("test.key"));
}

// =========================================================================
// RemoteConfiguration: 信号
// =========================================================================

void TestRemoteConfig::testRemoteConfigChangeSignal() {
    RemoteConfiguration config;

    QSignalSpy spy(&config, &RemoteConfiguration::configChanged);

    config.setString("test.key", "newvalue");

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QCOMPARE(args.at(0).toString(), QString("test.key"));
    QCOMPARE(args.at(1).toString(), QString("newvalue"));
}

void TestRemoteConfig::testRemoteConfigSyncedSignal() {
    RemoteConfiguration config;

    auto mock = std::make_unique<MockRemoteConfigSource>();
    mock->mockConfig["server.host"] = "prod.example.com";
    mock->mockConfig["server.port"] = 443;

    config.setSource(std::move(mock));

    QSignalSpy spy(&config, &RemoteConfiguration::configSynced);

    auto result = config.sync("myapp-config");
    QVERIFY(result.isOk());
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString("myapp-config"));

    // 验证从远程同步的数据
    QCOMPARE(config.getString("server.host"), QString("prod.example.com"));
    QCOMPARE(config.getInt("server.port"), 443);
}

// =========================================================================
// RemoteConfiguration: 文件加载/保存
// =========================================================================

void TestRemoteConfig::testRemoteConfigLoadFromFile() {
    QTemporaryFile file;
    file.open();
    file.write(R"({"host":"localhost","port":5432,"debug":true})");
    file.close();

    RemoteConfiguration config;
    auto result = config.load(file.fileName());
    QVERIFY(result.isOk());
    QCOMPARE(config.getString("host"), QString("localhost"));
    QCOMPARE(config.getInt("port"), 5432);
    QVERIFY(config.getBool("debug"));
}

void TestRemoteConfig::testRemoteConfigSaveToFile() {
    RemoteConfiguration config;
    config.setString("host", "localhost");
    config.setInt("port", 6379);
    config.setBool("enabled", true);

    QTemporaryFile file;
    file.open();
    file.close();

    auto result = config.save(file.fileName());
    QVERIFY(result.isOk());

    // 验证保存的文件
    QFile f(file.fileName());
    f.open(QIODevice::ReadOnly);
    QByteArray content = f.readAll();
    f.close();

    auto parseResult = sc::json::deserialize(content);
    QVERIFY(parseResult.isOk());
    sc::json::Json obj = parseResult.unwrap();
    QCOMPARE(sc::json::getString(obj, "host"), QString("localhost"));
    QCOMPARE(sc::json::getInt(obj, "port"), 6379);
    QVERIFY(sc::json::getBool(obj, "enabled"));
}

void TestRemoteConfig::testRemoteConfigLoadNonExistentFile() {
    RemoteConfiguration config;
    auto result = config.load("/nonexistent/path/file.json");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::NotFound);
}

void TestRemoteConfig::testRemoteConfigLoadInvalidJson() {
    QTemporaryFile file;
    file.open();
    file.write("not valid json {{{");
    file.close();

    RemoteConfiguration config;
    auto result = config.load(file.fileName());
    QVERIFY(result.isErr());
    // [v1.9.2] nlohmann/json 解析失败返回 DeserializationError(302)
    QCOMPARE(result.unwrapErr().code(), ErrorCode::DeserializationError);
}

// =========================================================================
// RemoteConfiguration: 远程源集成
// =========================================================================

void TestRemoteConfig::testRemoteConfigSetSource() {
    RemoteConfiguration config;
    auto mock = std::make_unique<MockRemoteConfigSource>();
    config.setSource(std::move(mock));
    // 设置源后不应崩溃
}

void TestRemoteConfig::testRemoteConfigSync() {
    RemoteConfiguration config;
    auto mock = std::make_unique<MockRemoteConfigSource>();
    mock->mockConfig["server.host"] = "test.example.com";
    mock->mockConfig["server.port"] = 8080;

    auto* rawMock = mock.get();
    config.setSource(std::move(mock));

    auto result = config.sync("myapp-config");
    QVERIFY(result.isOk());
    QCOMPARE(rawMock->lastFetchedNamespace, QString("myapp-config"));
    QCOMPARE(config.getString("server.host"), QString("test.example.com"));
    QCOMPARE(config.getInt("server.port"), 8080);
}

void TestRemoteConfig::testRemoteConfigSyncNoSource() {
    RemoteConfiguration config;
    auto result = config.sync("myapp-config");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::NotConnected);
}

void TestRemoteConfig::testRemoteConfigWatch() {
    RemoteConfiguration config;
    auto mock = std::make_unique<MockRemoteConfigSource>();
    mock->mockConfig["watch.key"] = "watched";

    auto* rawMock2 = mock.get();
    config.setSource(std::move(mock));

    auto result = config.watch("myapp-config");
    QVERIFY(result.isOk());
    QCOMPARE(rawMock2->lastFetchedNamespace, QString("myapp-config"));
    // watch 触发回调后会更新本地配置
    QCOMPARE(config.getString("watch.key"), QString("watched"));
}

void TestRemoteConfig::testRemoteConfigWatchNoSource() {
    RemoteConfiguration config;
    auto result = config.watch("myapp-config");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), ErrorCode::NotConnected);
}

// =========================================================================
// MockRemoteConfigSource
// =========================================================================

void TestRemoteConfig::testMockSourceConnect() {
    MockRemoteConfigSource source;
    QVERIFY(!source.connected);

    auto result = source.connectToServer();
    QVERIFY(result.isOk());
    QVERIFY(source.connected);
}

void TestRemoteConfig::testMockSourceFetch() {
    MockRemoteConfigSource source;
    source.mockConfig["key"] = "value";

    auto result = source.fetchConfig("test-ns");
    QVERIFY(result.isOk());
    QCOMPARE(source.lastFetchedNamespace, QString("test-ns"));

    sc::json::Json fetched = result.unwrap();
    QCOMPARE(sc::json::getString(fetched, "key"), QString("value"));
}

void TestRemoteConfig::testMockSourcePublish() {
    MockRemoteConfigSource source;
    sc::json::Json cfg = sc::json::Json::object();
    cfg["key"] = "pubval";

    auto result = source.publishConfig("test-ns", cfg);
    QVERIFY(result.isOk());
    QCOMPARE(source.lastPublishedNamespace, QString("test-ns"));
    QCOMPARE(sc::json::getString(source.lastPublishedConfig, "key"), QString("pubval"));
}

void TestRemoteConfig::testMockSourceDisconnect() {
    MockRemoteConfigSource source;
    QVERIFY(!source.disconnected);
    source.disconnectFromServer();
    QVERIFY(source.disconnected);
}

void TestRemoteConfig::testMockSourceFailures() {
    MockRemoteConfigSource source;

    source.failConnect = true;
    auto connResult = source.connectToServer();
    QVERIFY(connResult.isErr());
    QCOMPARE(connResult.unwrapErr().code(), ErrorCode::ConnectionRefused);

    source.failFetch = true;
    auto fetchResult = source.fetchConfig("test");
    QVERIFY(fetchResult.isErr());
    QCOMPARE(fetchResult.unwrapErr().code(), ErrorCode::NotFound);

    source.failPublish = true;
    auto pubResult = source.publishConfig("test", sc::json::Json::object());
    QVERIFY(pubResult.isErr());
    QCOMPARE(pubResult.unwrapErr().code(), ErrorCode::NetworkError);
}

// =========================================================================
// NacosConfigSource
// =========================================================================

void TestRemoteConfig::testNacosSourceConstruction() {
    NacosConfigSource source("http://192.168.1.100:8848", "MY_GROUP");
    // 构造不应崩溃
}

void TestRemoteConfig::testNacosSourceDefaultValues() {
    NacosConfigSource source;
    // 默认构造应使用默认值，不应崩溃
    // 验证析构时正确清理资源
}

// =========================================================================
// EtcdConfigSource
// =========================================================================

void TestRemoteConfig::testEtcdSourceConstruction() {
    EtcdConfigSource source("http://192.168.1.100:2379");
    // 构造不应崩溃
}

void TestRemoteConfig::testEtcdSourceDefaultValues() {
    EtcdConfigSource source;
    // 默认构造应使用默认值，不应崩溃
}

// =========================================================================
// IConfiguration 接口兼容性
// =========================================================================

void TestRemoteConfig::testRemoteConfigAsIConfiguration() {
    // 验证 RemoteConfiguration 可以作为 IConfiguration 使用
    std::shared_ptr<IConfiguration> config = std::make_shared<RemoteConfiguration>();

    config->setString("iface.key", "iface_val");
    config->setInt("iface.int", 100);
    config->setDouble("iface.dbl", 1.5);
    config->setBool("iface.bool", true);

    QCOMPARE(config->getString("iface.key"), QString("iface_val"));
    QCOMPARE(config->getInt("iface.int"), 100);
    QCOMPARE(config->getDouble("iface.dbl"), 1.5);
    QVERIFY(config->getBool("iface.bool"));

    QVERIFY(config->contains("iface.key"));
    QVERIFY(!config->contains("iface.missing"));

    config->remove("iface.key");
    QVERIFY(!config->contains("iface.key"));

    QCOMPARE(config->interfaceName(), std::string("IConfiguration"));
}

QTEST_MAIN(TestRemoteConfig)
#include "test_remote_config.moc"