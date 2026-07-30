// ============================================================================
// test_resource_pool_monitor.cpp — 资源池监控单元测试
// ============================================================================
//
// 覆盖范围:
//   - ThreadPoolMonitor: active/idle/max 计算
//   - DbConnectionPoolMonitor: active/idle/max 计算
//   - NetworkConnectionPoolMonitor: active/idle/max 计算
//   - ResourcePoolMonitorRegistry: 注册/注销/快照/告警
//   - ResourcePoolMetricsCollector: 启停/采集(不验证后台线程,避免 CI flaky)

#include <QTest>
#include <QString>
#include <chrono>
#include <memory>
#include <thread>

#include "soul/observability/resource_pool_monitor.h"
#include "soul/observability/metrics.h"
#include "soul/async/thread_pool.h"
#include "soul/network/pool/connection_pool.h"
#include "soul/data/connection_pool.h"
#include "soul/data/database_driver.h"

using namespace sc;
using namespace sc::observability;

// ============================================================================
// 自定义桩资源池(用于精确控制 active/idle/max 验证计算逻辑)
// ============================================================================
class StubResourcePoolMonitor : public IResourcePoolMonitor {
public:
    std::string n;
    int active = 0;
    int idle   = 0;
    int maxc   = 0;

    explicit StubResourcePoolMonitor(std::string name, int a, int i, int m)
        : n(std::move(name)), active(a), idle(i), maxc(m) {}

    std::string name() const override { return n; }
    int activeCount() const override { return active; }
    int idleCount() const override { return idle; }
    int maxCount() const override { return maxc; }
};

// ============================================================================
// TestResourcePoolMonitor — 资源池监控测试
// ============================================================================
class TestResourcePoolMonitor : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void testStubSnapshot();
    void testSnapshotUtilizationCalculation();
    void testRegistryRegisterUnregister();
    void testRegistrySnapshots();
    void testAlertThreshold();
    void testThreadPoolMonitor();
    void testDbConnectionPoolMonitor();
    void testNetworkConnectionPoolMonitor();
    void testMetricsCollectorStartStop();
    void testMetricsCollectorCollectOnce();
};

void TestResourcePoolMonitor::initTestCase() {
    ResourcePoolMonitorRegistry::instance().clear();
    MetricsRegistry::instance().clear();
}

void TestResourcePoolMonitor::cleanupTestCase() {
    ResourcePoolMonitorRegistry::instance().clear();
    MetricsRegistry::instance().clear();
}

// 验证 snapshot() 正确聚合字段
void TestResourcePoolMonitor::testStubSnapshot() {
    StubResourcePoolMonitor stub("stub1", 3, 2, 10);
    const auto s = stub.snapshot();
    QCOMPARE(QString::fromStdString(s.name), QString("stub1"));
    QCOMPARE(s.activeCount, 3);
    QCOMPARE(s.idleCount, 2);
    QCOMPARE(s.maxCount, 10);
}

// 验证利用率计算 = active / max
void TestResourcePoolMonitor::testSnapshotUtilizationCalculation() {
    StubResourcePoolMonitor stub("stub2", 5, 5, 10);
    QCOMPARE(stub.snapshot().utilization, 0.5);

    StubResourcePoolMonitor full("stub_full", 10, 0, 10);
    QCOMPARE(full.snapshot().utilization, 1.0);

    StubResourcePoolMonitor empty("stub_empty", 0, 10, 10);
    QCOMPARE(empty.snapshot().utilization, 0.0);

    // max=0 时利用率为 0(避免除零)
    StubResourcePoolMonitor nomax("stub_nomax", 5, 0, 0);
    QCOMPARE(nomax.snapshot().utilization, 0.0);
}

// 验证注册表注册/注销
void TestResourcePoolMonitor::testRegistryRegisterUnregister() {
    ResourcePoolMonitorRegistry::instance().clear();
    auto stub1 = std::make_shared<StubResourcePoolMonitor>("pool1", 1, 1, 5);
    auto stub2 = std::make_shared<StubResourcePoolMonitor>("pool2", 2, 2, 8);

    ResourcePoolMonitorRegistry::instance().registerMonitor(stub1);
    ResourcePoolMonitorRegistry::instance().registerMonitor(stub2);

    const auto names = ResourcePoolMonitorRegistry::instance().names();
    QCOMPARE(static_cast<int>(names.size()), 2);

    ResourcePoolMonitorRegistry::instance().unregisterMonitor("pool1");
    const auto namesAfter = ResourcePoolMonitorRegistry::instance().names();
    QCOMPARE(static_cast<int>(namesAfter.size()), 1);
    QCOMPARE(QString::fromStdString(namesAfter[0]), QString("pool2"));

    ResourcePoolMonitorRegistry::instance().clear();
}

// 验证批量快照
void TestResourcePoolMonitor::testRegistrySnapshots() {
    ResourcePoolMonitorRegistry::instance().clear();
    ResourcePoolMonitorRegistry::instance().registerMonitor(
        std::make_shared<StubResourcePoolMonitor>("a", 1, 2, 5));
    ResourcePoolMonitorRegistry::instance().registerMonitor(
        std::make_shared<StubResourcePoolMonitor>("b", 3, 4, 10));

    const auto snaps = ResourcePoolMonitorRegistry::instance().snapshots();
    QCOMPARE(static_cast<int>(snaps.size()), 2);
    // 快照顺序不保证,按名称查找校验
    bool foundA = false, foundB = false;
    for (const auto& s : snaps) {
        if (s.name == "a") {
            QCOMPARE(s.activeCount, 1);
            foundA = true;
        }
        if (s.name == "b") {
            QCOMPARE(s.activeCount, 3);
            foundB = true;
        }
    }
    QVERIFY(foundA && foundB);
    ResourcePoolMonitorRegistry::instance().clear();
}

// 验证阈值告警回调
void TestResourcePoolMonitor::testAlertThreshold() {
    ResourcePoolMonitorRegistry::instance().clear();
    ResourcePoolMonitorRegistry::instance().registerMonitor(
        std::make_shared<StubResourcePoolMonitor>("high", 9, 1, 10));   // 90%
    ResourcePoolMonitorRegistry::instance().registerMonitor(
        std::make_shared<StubResourcePoolMonitor>("low", 1, 9, 10));    // 10%

    int alertCount = 0;
    std::string alertedName;
    ResourcePoolMonitorRegistry::instance().setAlertThreshold(0.8,
        [&alertCount, &alertedName](const std::string& name, double util) {
            ++alertCount;
            alertedName = name;
            QVERIFY(util > 0.8);
        });

    ResourcePoolMonitorRegistry::instance().checkAlerts();
    QCOMPARE(alertCount, 1);
    QCOMPARE(QString::fromStdString(alertedName), QString("high"));

    ResourcePoolMonitorRegistry::instance().clear();
}

// 验证 ThreadPoolMonitor 适配器
void TestResourcePoolMonitor::testThreadPoolMonitor() {
    ThreadPool& pool = ThreadPool::instance();
    pool.init(4);  // 4 线程
    ThreadPoolMonitor monitor(pool, "TestThreadPool");
    QCOMPARE(QString::fromStdString(monitor.name()), QString("TestThreadPool"));
    QCOMPARE(monitor.maxCount(), 4);
    // activeCount/idleCount 应满足 active + idle <= max
    const int active = monitor.activeCount();
    const int idle = monitor.idleCount();
    QVERIFY(active >= 0);
    QVERIFY(idle >= 0);
    QVERIFY(active + idle <= 4);
    pool.shutdown();
}

// 验证 DbConnectionPoolMonitor 适配器
void TestResourcePoolMonitor::testDbConnectionPoolMonitor() {
    // 使用内存数据库驱动工厂创建连接池
    data::ConnectionConfig config;
    config.type = data::DatabaseType::SQLite;
    config.database = ":memory:";
    data::DefaultDbConnectionPool pool(
        [config]() -> std::unique_ptr<data::IDatabaseDriver> {
            auto drv = data::DatabaseDriverFactory::instance().create(config.type);
            if (drv) { drv->open(config); }
            return drv;
        },
        1, 5);
    pool.initialize();

    DbConnectionPoolMonitor monitor(pool, 5, "TestDbPool");
    QCOMPARE(QString::fromStdString(monitor.name()), QString("TestDbPool"));
    QCOMPARE(monitor.maxCount(), 5);
    // 初始状态:active=0
    QCOMPARE(monitor.activeCount(), 0);

    // acquire 一个连接,active 应变为 1
    auto conn = pool.acquire();
    QVERIFY(conn.isOk());
    QCOMPARE(monitor.activeCount(), 1);

    // release 后 active 应回到 0
    pool.release(std::move(conn.unwrap()));
    QCOMPARE(monitor.activeCount(), 0);

    pool.closeAll();
}

// 验证 NetworkConnectionPoolMonitor 适配器
void TestResourcePoolMonitor::testNetworkConnectionPoolMonitor() {
    network::ConnectionPool::Config cfg;
    cfg.maxConnections = 8;
    cfg.minConnections = 1;
    network::ConnectionPool pool(cfg);

    NetworkConnectionPoolMonitor monitor(pool, "TestNetPool");
    QCOMPARE(QString::fromStdString(monitor.name()), QString("TestNetPool"));
    QCOMPARE(monitor.maxCount(), 8);
    // 初始无连接
    QCOMPARE(monitor.activeCount(), 0);
    QCOMPARE(monitor.idleCount(), 0);
}

// 验证采集器启停(幂等)
void TestResourcePoolMonitor::testMetricsCollectorStartStop() {
    ResourcePoolMetricsCollector collector(std::chrono::milliseconds(100));
    QVERIFY(!collector.isRunning());

    collector.start();
    QVERIFY(collector.isRunning());

    // 重复 start 幂等
    collector.start();
    QVERIFY(collector.isRunning());

    collector.stop();
    QVERIFY(!collector.isRunning());

    // 重复 stop 幂等
    collector.stop();
    QVERIFY(!collector.isRunning());
}

// 验证采集器 collectOnce 更新 MetricsRegistry
void TestResourcePoolMonitor::testMetricsCollectorCollectOnce() {
    ResourcePoolMonitorRegistry::instance().clear();
    MetricsRegistry::instance().clear();

    ResourcePoolMonitorRegistry::instance().registerMonitor(
        std::make_shared<StubResourcePoolMonitor>("collect_test", 2, 3, 10));

    ResourcePoolMetricsCollector collector(std::chrono::seconds(60));
    collector.collectOnce();

    // 验证 Gauge 已写入 labeled 值
    auto& activeGauge = MetricsRegistry::instance().gauge(
        "resource_pool_active_count", "Active resource count of resource pools");
    const auto labeled = activeGauge.labeledValues();
    QVERIFY(!labeled.empty());

    ResourcePoolMonitorRegistry::instance().clear();
    MetricsRegistry::instance().clear();
}

QTEST_GUILESS_MAIN(TestResourcePoolMonitor)
#include "test_resource_pool_monitor.moc"
