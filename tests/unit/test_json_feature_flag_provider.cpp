#include <QTest>
#include <QTemporaryFile>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include "soul/core/json_feature_flag_provider.h"

using namespace sc;

// ============================================================================
// 辅助: 写入 JSON 配置文件
// ============================================================================
static QString writeTempConfig(const QString& json) {
    QTemporaryFile* f = new QTemporaryFile();  // autoRemove=false 以保持文件
    f->setAutoRemove(false);
    f->open();
    f->write(json.toUtf8());
    f->close();
    return f->fileName();
}

// ============================================================================
// TestJsonFeatureFlagProvider
// ============================================================================
class TestJsonFeatureFlagProvider : public QObject {
    Q_OBJECT

private slots:
    void testInitializeLoadsFlags();
    void testGetConfigBoolean();
    void testGetConfigPercentage();
    void testGetConfigTargeted();
    void testGetConfigScheduled();
    void testGetConfigRuleBased();
    void testGetConfigKillSwitch();
    void testGetConfigNotFound();
    void testGetAllConfigs();
    void testSetConfig();
    void testDeleteConfig();
    void testNotInitializedError();

private:
    QString m_tempPath;
};

void TestJsonFeatureFlagProvider::testInitializeLoadsFlags() {
    QString json = R"({
        "flags": {
            "new_checkout": { "type": "boolean", "enabled": true },
            "dark_mode":    { "type": "percentage", "percentage": 30 }
        }
    })";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    auto result = provider.initialize();
    QVERIFY(result.isOk());

    auto configs = provider.getAllConfigs();
    QVERIFY(configs.isOk());
    QCOMPARE(configs.unwrap().size(), 2);
    QVERIFY(configs.unwrap().contains("new_checkout"));
    QVERIFY(configs.unwrap().contains("dark_mode"));

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetConfigBoolean() {
    QString json = R"({"flags": {"my_flag": {"type": "boolean", "enabled": true}}})";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto cfg = provider.getConfig("my_flag");
    QVERIFY(cfg.isOk());
    QCOMPARE(cfg.unwrap().type, FeatureFlagType::Boolean);
    QVERIFY(cfg.unwrap().enabled);

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetConfigPercentage() {
    QString json = R"({"flags": {"dark_mode": {"type": "percentage", "percentage": 50}}})";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto cfg = provider.getConfig("dark_mode");
    QVERIFY(cfg.isOk());
    QCOMPARE(cfg.unwrap().type, FeatureFlagType::Percentage);
    QCOMPARE(cfg.unwrap().percentage, 50);

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetConfigTargeted() {
    QString json = R"({
        "flags": {
            "admin_only": {
                "type": "targeted",
                "allowedRoles": ["admin", "superadmin"],
                "allowedUsers": ["user1"]
            }
        }
    })";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto cfg = provider.getConfig("admin_only");
    QVERIFY(cfg.isOk());
    QCOMPARE(cfg.unwrap().type, FeatureFlagType::Targeted);
    QVERIFY(cfg.unwrap().allowedRoles.contains("admin"));
    QVERIFY(cfg.unwrap().allowedRoles.contains("superadmin"));
    QVERIFY(cfg.unwrap().allowedUsers.contains("user1"));

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetConfigScheduled() {
    QString json = R"({
        "flags": {
            "holiday_promo": {
                "type": "scheduled",
                "enabled": true,
                "startTime": "2026-12-24T00:00:00",
                "endTime": "2026-12-26T23:59:59"
            }
        }
    })";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto cfg = provider.getConfig("holiday_promo");
    QVERIFY(cfg.isOk());
    QCOMPARE(cfg.unwrap().type, FeatureFlagType::Scheduled);
    QVERIFY(cfg.unwrap().startTime.isValid());
    QVERIFY(cfg.unwrap().endTime.isValid());

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetConfigRuleBased() {
    QString json = R"({
        "flags": {
            "vip_access": {
                "type": "rule_based",
                "ruleLogic": "OR",
                "rules": [
                    {"attribute": "role", "op": "eq", "value": "vip"},
                    {"attribute": "level", "op": "gt", "value": "10"}
                ]
            }
        }
    })";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto cfg = provider.getConfig("vip_access");
    QVERIFY(cfg.isOk());
    QCOMPARE(cfg.unwrap().type, FeatureFlagType::RuleBased);
    QCOMPARE(cfg.unwrap().rules.size(), static_cast<size_t>(2));
    QCOMPARE(cfg.unwrap().ruleLogic, QString("OR"));

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetConfigKillSwitch() {
    QString json = R"({"flags": {"emergency_off": {"type": "killswitch", "enabled": true}}})";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto cfg = provider.getConfig("emergency_off");
    QVERIFY(cfg.isOk());
    QCOMPARE(cfg.unwrap().type, FeatureFlagType::KillSwitch);
    QVERIFY(cfg.unwrap().enabled);

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetConfigNotFound() {
    QString json = R"({"flags": {"only_this": {"type": "boolean", "enabled": false}}})";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto cfg = provider.getConfig("does_not_exist");
    QVERIFY(cfg.isErr());

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testGetAllConfigs() {
    QString json = R"({
        "flags": {
            "flag_a": {"type": "boolean", "enabled": true},
            "flag_b": {"type": "percentage", "percentage": 75},
            "flag_c": {"type": "targeted", "allowedRoles": ["admin"]}
        }
    })";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto configs = provider.getAllConfigs();
    QVERIFY(configs.isOk());
    QCOMPARE(configs.unwrap().size(), 3);

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testSetConfig() {
    QString json = R"({"flags": {}})";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    FeatureFlagConfig cfg;
    cfg.type = FeatureFlagType::Boolean;
    cfg.enabled = true;
    cfg.description = "test flag";
    auto result = provider.setConfig("new_flag", cfg);
    QVERIFY(result.isOk());

    auto retrieved = provider.getConfig("new_flag");
    QVERIFY(retrieved.isOk());
    QCOMPARE(retrieved.unwrap().description, QString("test flag"));

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testDeleteConfig() {
    QString json = R"({"flags": {"to_delete": {"type": "boolean", "enabled": false}}})";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    provider.initialize();

    auto result = provider.deleteConfig("to_delete");
    QVERIFY(result.isOk());

    auto cfg = provider.getConfig("to_delete");
    QVERIFY(cfg.isErr());

    provider.shutdown();
    QFile::remove(m_tempPath);
}

void TestJsonFeatureFlagProvider::testNotInitializedError() {
    QString json = R"({"flags": {}})";
    m_tempPath = writeTempConfig(json);

    JsonFeatureFlagProvider provider(m_tempPath, false);
    // 不调用 initialize

    auto cfg = provider.getConfig("any");
    QVERIFY(cfg.isErr());

    QFile::remove(m_tempPath);
}

QTEST_MAIN(TestJsonFeatureFlagProvider)
#include "test_json_feature_flag_provider.moc"
