#include <QTest>
#include <QCoreApplication>

#include "soul/configuration/config.h"
#include "soul/configuration/config_bind.h"

using namespace sc;

// ============================================================================
// 测试用配置结构体
// ============================================================================

struct ServerConfig {
    QString host;
    int port = 0;
    bool tls = false;
    double timeout = 0.0;
};

SC_CONFIG_BIND(ServerConfig,
    SC_CFG_FIELD("host", &ServerConfig::host, "localhost")
    SC_CFG_FIELD("port", &ServerConfig::port, 8080)
    SC_CFG_FIELD("tls",  &ServerConfig::tls,  false)
    SC_CFG_FIELD("timeout", &ServerConfig::timeout, 30.0)
)

struct DatabaseConfig {
    QString driver;
    QString connectionName;
    int maxConnections = 0;
};

SC_CONFIG_BIND_PREFIX(DatabaseConfig, "database",
    SC_CFG_FIELD("driver", &DatabaseConfig::driver, "QSQLITE")
    SC_CFG_FIELD("connectionName", &DatabaseConfig::connectionName, "default")
    SC_CFG_FIELD("maxConnections", &DatabaseConfig::maxConnections, 10)
)

// 无反射绑定的结构体(测试默认行为)
struct EmptyConfig {
    int value = 42;
};

// ============================================================================
// TestConfigBind — 配置绑定测试
// ============================================================================
class TestConfigBind : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        Config::instance().init();
    }

    void cleanupTestCase() {
        Config::instance().shutdown();
    }

    void cleanup() {
        // 清空配置
        Config::instance().remove("server.host");
        Config::instance().remove("server.port");
        Config::instance().remove("server.tls");
        Config::instance().remove("server.timeout");
        Config::instance().remove("database.driver");
        Config::instance().remove("database.connectionName");
        Config::instance().remove("database.maxConnections");
    }

    // 默认值绑定
    void testDefaultValues() {
        auto cfg = Config::instance().bind<ServerConfig>("server");
        QCOMPARE(cfg.host, QString("localhost"));
        QCOMPARE(cfg.port, 8080);
        QCOMPARE(cfg.tls, false);
        QCOMPARE(cfg.timeout, 30.0);
    }

    // 自定义值绑定
    void testCustomValues() {
        Config::instance().setString("server.host", "192.168.1.1");
        Config::instance().setInt("server.port", 9090);
        Config::instance().setBool("server.tls", true);
        Config::instance().setDouble("server.timeout", 60.0);

        auto cfg = Config::instance().bind<ServerConfig>("server");
        QCOMPARE(cfg.host, QString("192.168.1.1"));
        QCOMPARE(cfg.port, 9090);
        QCOMPARE(cfg.tls, true);
        QCOMPARE(cfg.timeout, 60.0);
    }

    // 部分覆盖
    void testPartialOverride() {
        Config::instance().setInt("server.port", 3000);
        // host 未设置,使用默认值

        auto cfg = Config::instance().bind<ServerConfig>("server");
        QCOMPARE(cfg.host, QString("localhost"));  // 默认值
        QCOMPARE(cfg.port, 3000);                  // 覆盖值
        QCOMPARE(cfg.tls, false);                   // 默认值
    }

    // 前缀绑定(使用 SC_CONFIG_BIND_PREFIX)
    void testPrefixBinding() {
        Config::instance().setString("database.driver", "QMYSQL");
        Config::instance().setString("database.connectionName", "prod");
        Config::instance().setInt("database.maxConnections", 50);

        // 使用 traits 中声明的 prefix(不传参)
        auto cfg = Config::instance().bind<DatabaseConfig>();
        QCOMPARE(cfg.driver, QString("QMYSQL"));
        QCOMPARE(cfg.connectionName, QString("prod"));
        QCOMPARE(cfg.maxConnections, 50);
    }

    // 前缀绑定(默认值)
    void testPrefixBindingDefaults() {
        auto cfg = Config::instance().bind<DatabaseConfig>();
        QCOMPARE(cfg.driver, QString("QSQLITE"));
        QCOMPARE(cfg.connectionName, QString("default"));
        QCOMPARE(cfg.maxConnections, 10);
    }

    // 无反射绑定结构体
    void testNoReflection() {
        auto cfg = Config::instance().bind<EmptyConfig>("empty");
        QCOMPARE(cfg.value, 42);  // 保持原始默认值
    }

    // 空 prefix
    void testEmptyPrefix() {
        Config::instance().setInt("port", 1234);

        auto cfg = Config::instance().bind<ServerConfig>("");
        // 空 prefix: key 直接使用 fieldName 不加前缀
        QCOMPARE(cfg.port, 1234);
        QCOMPARE(cfg.host, QString("localhost"));  // 默认值
    }

    // 多次绑定返回独立实例
    void testIndependentInstances() {
        Config::instance().setString("server.host", "host1");
        auto cfg1 = Config::instance().bind<ServerConfig>("server");

        Config::instance().setString("server.host", "host2");
        auto cfg2 = Config::instance().bind<ServerConfig>("server");

        QCOMPARE(cfg1.host, QString("host1"));
        QCOMPARE(cfg2.host, QString("host2"));
    }
};

QTEST_GUILESS_MAIN(TestConfigBind)
#include "test_config_bind.moc"