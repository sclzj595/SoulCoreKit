#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include "soul/core/feature_flags.h"

using namespace sc;

// ============================================================================
// TestFeatureFlagManager — 功能开关管理器单元测试
// ============================================================================
class TestFeatureFlagManager : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingletonInstance();
    void testInitializeShutdown();
    void testRegisterFlagBoolean();
    void testIsEnabledBoolean();
    void testSetFlag();
    void testRemoveFlag();
    void testForceEnableForceDisable();
    void testIsForced();
    void testEvaluateAll();
    void testEvaluateWithTarget();
    void testOnFlagChanged();
    void testRemoveFlagListener();
    void testGetAllSnapshots();
};

void TestFeatureFlagManager::initTestCase() {
}

void TestFeatureFlagManager::cleanupTestCase() {
    FeatureFlagManager::instance().shutdown();
}

void TestFeatureFlagManager::testSingletonInstance() {
    FeatureFlagManager& mgr1 = FeatureFlagManager::instance();
    FeatureFlagManager& mgr2 = FeatureFlagManager::instance();
    QCOMPARE(&mgr1, &mgr2);
}

void TestFeatureFlagManager::testInitializeShutdown() {
    auto& mgr = FeatureFlagManager::instance();
    // shutdown 后再初始化应该 OK
    mgr.shutdown();
    auto result = mgr.initialize(nullptr);
    // 没有 provider 时 initialize 也应该 OK（本地模式）
    QVERIFY(result.isOk());
}

void TestFeatureFlagManager::testRegisterFlagBoolean() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.type = FeatureFlagType::Boolean;
    config.enabled = true;
    config.description = "Test boolean flag";

    auto r = mgr.setFlag("test.boolean.flag", config);
    QVERIFY(r.isOk());
}

void TestFeatureFlagManager::testIsEnabledBoolean() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.type = FeatureFlagType::Boolean;
    config.enabled = true;
    (void)mgr.setFlag("test.enabled.flag", config);

    QVERIFY(mgr.isEnabled("test.enabled.flag"));

    // 不存在的 flag 默认返回 false
    QVERIFY(!mgr.isEnabled("nonexistent.flag"));
}

void TestFeatureFlagManager::testSetFlag() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.type = FeatureFlagType::Boolean;
    config.enabled = true;
    config.description = "My flag";

    auto r = mgr.setFlag("my.flag", config);
    QVERIFY(r.isOk());

    auto fetched = mgr.getFlagConfig("my.flag");
    QVERIFY(fetched.isOk());
    QCOMPARE(fetched.unwrap().description, QString("My flag"));
}

void TestFeatureFlagManager::testRemoveFlag() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.enabled = true;
    (void)mgr.setFlag("temp.flag", config);

    auto r = mgr.removeFlag("temp.flag");
    QVERIFY(r.isOk());

    // 移除后再查询应该失败
    auto fetched = mgr.getFlagConfig("temp.flag");
    QVERIFY(!fetched.isOk());
}

void TestFeatureFlagManager::testForceEnableForceDisable() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.enabled = false;
    (void)mgr.setFlag("force.test.flag", config);

    // 原始值为 false
    QVERIFY(!mgr.isEnabled("force.test.flag"));

    // forceEnable 后变为 true
    mgr.forceEnable("force.test.flag");
    QVERIFY(mgr.isEnabled("force.test.flag"));

    // forceDisable 后变为 false
    mgr.forceDisable("force.test.flag");
    QVERIFY(!mgr.isEnabled("force.test.flag"));

    // removeForce 恢复原始值
    mgr.removeForce("force.test.flag");
    // 原始值是 false，所以恢复后仍然 false
    QVERIFY(!mgr.isEnabled("force.test.flag"));
}

void TestFeatureFlagManager::testIsForced() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.enabled = true;
    (void)mgr.setFlag("forced.check.flag", config);

    QVERIFY(!mgr.isForced("forced.check.flag"));

    mgr.forceEnable("forced.check.flag");
    QVERIFY(mgr.isForced("forced.check.flag"));

    mgr.removeForce("forced.check.flag");
    QVERIFY(!mgr.isForced("forced.check.flag"));
}

void TestFeatureFlagManager::testEvaluateAll() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config1;
    config1.enabled = true;
    (void)mgr.setFlag("eval.all.1", config1);

    FeatureFlagConfig config2;
    config2.enabled = false;
    (void)mgr.setFlag("eval.all.2", config2);

    auto results = mgr.evaluateAll();
    QVERIFY(results.contains("eval.all.1"));
    QVERIFY(results.contains("eval.all.2"));
    QVERIFY(results["eval.all.1"]);
    QVERIFY(!results["eval.all.2"]);
}

void TestFeatureFlagManager::testEvaluateWithTarget() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.type = FeatureFlagType::Targeted;
    config.enabled = false;
    config.allowedUsers.insert("user123");
    config.allowedRoles.insert("admin");
    (void)mgr.setFlag("targeted.flag", config);

    // 无目标用户时，Targeted 类型且 enabled=false 则返回 false
    FeatureFlagTarget target;
    QVERIFY(!mgr.isEnabled("targeted.flag", target));

    // 匹配用户
    target.userId = "user123";
    QVERIFY(mgr.isEnabled("targeted.flag", target));

    // 匹配角色
    FeatureFlagTarget target2;
    target2.role = "admin";
    QVERIFY(mgr.isEnabled("targeted.flag", target2));
}

void TestFeatureFlagManager::testOnFlagChanged() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.enabled = false;
    (void)mgr.setFlag("change.flag", config);

    int changeCount = 0;
    QString lastKey;
    bool lastValue = false;

    mgr.onFlagChanged("change.flag", [&](const QString& key, bool newValue) {
        changeCount++;
        lastKey = key;
        lastValue = newValue;
    });

    // 修改 flag 触发回调
    FeatureFlagConfig newConfig;
    newConfig.enabled = true;
    (void)mgr.setFlag("change.flag", newConfig);

    QCOMPARE(changeCount, 1);
    QCOMPARE(lastKey, QString("change.flag"));
    QVERIFY(lastValue);
}

void TestFeatureFlagManager::testRemoveFlagListener() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config;
    config.enabled = false;
    (void)mgr.setFlag("listener.flag", config);

    int changeCount = 0;
    mgr.onFlagChanged("listener.flag", [&](const QString&, bool) {
        changeCount++;
    });

    mgr.removeFlagListener("listener.flag");

    FeatureFlagConfig newConfig;
    newConfig.enabled = true;
    (void)mgr.setFlag("listener.flag", newConfig);

    // 移除监听后不应触发回调
    QCOMPARE(changeCount, 0);
}

void TestFeatureFlagManager::testGetAllSnapshots() {
    auto& mgr = FeatureFlagManager::instance();
    mgr.shutdown();

    FeatureFlagConfig config1;
    config1.enabled = true;
    (void)mgr.setFlag("snap.1", config1);

    FeatureFlagConfig config2;
    config2.enabled = false;
    (void)mgr.setFlag("snap.2", config2);

    auto snapshots = mgr.getAllSnapshots();
    QVERIFY(snapshots.contains("snap.1"));
    QVERIFY(snapshots.contains("snap.2"));
    QVERIFY(snapshots["snap.1"].currentValue);
    QVERIFY(!snapshots["snap.2"].currentValue);
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestFeatureFlagManager test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_feature_flags.moc"