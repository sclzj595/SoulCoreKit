#include <QTest>
#include <QSignalSpy>
#include <QJsonObject>
#include <QApplication>

#include "soul/cs/cs_admin_panel.h"

using namespace sc::cs;

// ============================================================================
// TestCsAdminPanel — 管理后台面板单元测试
// ============================================================================
class TestCsAdminPanel : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testConstructor();
    void testSetDataProviders();
    void testRefreshAll();
    void testSetAutoRefreshInterval();
    void testStartStopAutoRefresh();
};

void TestCsAdminPanel::initTestCase() {
}

void TestCsAdminPanel::cleanupTestCase() {
}

void TestCsAdminPanel::testConstructor() {
    CsAdminPanel panel;
    QVERIFY(panel.tabWidget() != nullptr);
    QCOMPARE(panel.tabWidget()->count(), 5);  // 5 个面板: Health/Metrics/Info/Env/ThreadDump
}

void TestCsAdminPanel::testSetDataProviders() {
    CsAdminPanel panel;

    bool healthCalled = false;
    panel.setHealthEndpoint([&]() -> QJsonObject {
        healthCalled = true;
        QJsonObject obj;
        obj["status"] = "UP";
        return obj;
    });

    bool metricsCalled = false;
    panel.setMetricsEndpoint([&]() -> QJsonObject {
        metricsCalled = true;
        QJsonObject obj;
        obj["cpu"] = 0.5;
        return obj;
    });

    bool infoCalled = false;
    panel.setInfoEndpoint([&]() -> QJsonObject {
        infoCalled = true;
        QJsonObject obj;
        obj["app"] = "SoulCoreKit";
        return obj;
    });

    bool envCalled = false;
    panel.setEnvEndpoint([&]() -> QJsonObject {
        envCalled = true;
        QJsonObject obj;
        obj["JAVA_HOME"] = "/usr/java";
        return obj;
    });

    bool threadDumpCalled = false;
    panel.setThreadDumpEndpoint([&]() -> QJsonObject {
        threadDumpCalled = true;
        QJsonObject obj;
        obj["threads"] = 10;
        return obj;
    });

    // 验证数据提供者已设置（通过 refreshAll 触发）
    panel.refreshAll();
    QVERIFY(healthCalled);
    QVERIFY(metricsCalled);
    QVERIFY(infoCalled);
    QVERIFY(envCalled);
    QVERIFY(threadDumpCalled);
}

void TestCsAdminPanel::testRefreshAll() {
    CsAdminPanel panel;

    bool refreshed = false;
    panel.setHealthEndpoint([&]() -> QJsonObject {
        refreshed = true;
        return QJsonObject();
    });

    panel.refreshAll();
    QVERIFY(refreshed);
}

void TestCsAdminPanel::testSetAutoRefreshInterval() {
    CsAdminPanel panel;

    // 设置刷新间隔应该不报错
    panel.setAutoRefreshInterval(3000);
    // 二次设置
    panel.setAutoRefreshInterval(10000);
    // 验证: 不崩溃即通过
    QVERIFY(true);
}

void TestCsAdminPanel::testStartStopAutoRefresh() {
    CsAdminPanel panel;

    // 启动自动刷新
    panel.startAutoRefresh();
    // 停止自动刷新
    panel.stopAutoRefresh();
    // 再次启动
    panel.startAutoRefresh();
    panel.stopAutoRefresh();
    // 验证: 不崩溃即通过
    QVERIFY(true);
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    TestCsAdminPanel test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_cs_admin_panel.moc"