#include <QTest>
#include <QCoreApplication>

#include "soul/server/health.h"

using namespace sc::server;

// ============================================================================
// HealthStatus 测试
// ============================================================================
class TestHealthStatus : public QObject {
    Q_OBJECT
private slots:
    void testToString();
};

void TestHealthStatus::testToString() {
    QCOMPARE(QString(toString(HealthStatus::UP)), QString("UP"));
    QCOMPARE(QString(toString(HealthStatus::DOWN)), QString("DOWN"));
    QCOMPARE(QString(toString(HealthStatus::UNKNOWN)), QString("UNKNOWN"));
}

// ============================================================================
// HealthDetail 测试
// ============================================================================
class TestHealthDetail : public QObject {
    Q_OBJECT
private slots:
    void testDefaultValues();
    void testCustomValues();
};

void TestHealthDetail::testDefaultValues() {
    HealthDetail detail;
    QVERIFY(detail.name.empty());
    QCOMPARE(detail.status, HealthStatus::UNKNOWN);
    QVERIFY(detail.message.empty());
    QCOMPARE(detail.responseTime.count(), static_cast<long long>(0));
}

void TestHealthDetail::testCustomValues() {
    HealthDetail detail;
    detail.name = "database";
    detail.status = HealthStatus::UP;
    detail.message = "Connection pool active: 3/20";
    detail.responseTime = std::chrono::milliseconds(15);

    QCOMPARE(detail.name, std::string("database"));
    QCOMPARE(detail.status, HealthStatus::UP);
    QCOMPARE(detail.message, std::string("Connection pool active: 3/20"));
    QCOMPARE(detail.responseTime.count(), static_cast<long long>(15));
}

// ============================================================================
// HealthReport 测试
// ============================================================================
class TestHealthReport : public QObject {
    Q_OBJECT
private slots:
    void testToJsonEmpty();
    void testToJsonWithDetails();
    void testToJsonOverallDown();
};

void TestHealthReport::testToJsonEmpty() {
    HealthReport report;
    report.overall = HealthStatus::UP;
    QByteArray json = report.toJson();
    QVERIFY(json.contains("\"status\":\"UP\""));
    QVERIFY(json.contains("\"details\""));
}

void TestHealthReport::testToJsonWithDetails() {
    HealthReport report;
    report.overall = HealthStatus::UP;

    HealthDetail db;
    db.name = "database";
    db.status = HealthStatus::UP;
    db.message = "OK";
    db.responseTime = std::chrono::milliseconds(5);
    report.details.push_back(db);

    HealthDetail mq;
    mq.name = "mq";
    mq.status = HealthStatus::UP;
    mq.message = "Connected";
    mq.responseTime = std::chrono::milliseconds(3);
    report.details.push_back(mq);

    QByteArray json = report.toJson();
    QVERIFY(json.contains("\"database\""));
    QVERIFY(json.contains("\"mq\""));
    QVERIFY(json.contains("\"OK\""));
    QVERIFY(json.contains("\"Connected\""));
}

void TestHealthReport::testToJsonOverallDown() {
    HealthReport report;
    report.overall = HealthStatus::DOWN;

    HealthDetail db;
    db.name = "database";
    db.status = HealthStatus::DOWN;
    db.message = "Connection refused";
    report.details.push_back(db);

    QByteArray json = report.toJson();
    QVERIFY(json.contains("\"status\":\"DOWN\""));
    QVERIFY(json.contains("\"Connection refused\""));
}

// ============================================================================
// IHealthIndicator 自定义指示器测试
// ============================================================================
class TestCustomIndicator : public QObject {
    Q_OBJECT
private slots:
    void testUpIndicator();
    void testDownIndicator();
    void testCriticalIndicator();
};

void TestCustomIndicator::testUpIndicator() {
    // 使用 lambda 创建简单指示器
    auto upIndicator = std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "test_db");

    HealthDetail detail = upIndicator->check();
    QCOMPARE(detail.name, std::string("test_db"));
    QCOMPARE(detail.status, HealthStatus::UP);
    QVERIFY(detail.message.find("healthy") != std::string::npos);
    QVERIFY(detail.responseTime.count() >= 0);
    QVERIFY(upIndicator->isCritical());
}

void TestCustomIndicator::testDownIndicator() {
    auto downIndicator = std::make_shared<DatabaseHealthIndicator>(
        []() { return false; }, "bad_db");

    HealthDetail detail = downIndicator->check();
    QCOMPARE(detail.status, HealthStatus::DOWN);
    QVERIFY(detail.message.find("failed") != std::string::npos);
}

void TestCustomIndicator::testCriticalIndicator() {
    auto db = std::make_shared<DatabaseHealthIndicator>(
        []() { return true; });
    QVERIFY(db->isCritical());

    auto mq = std::make_shared<MqHealthIndicator>(
        []() { return true; });
    QVERIFY(!mq->isCritical());  // MQ 非关键

    auto network = std::make_shared<NetworkHealthIndicator>(
        []() { return true; });
    QVERIFY(!network->isCritical());  // 网络非关键
}

// ============================================================================
// HealthEndpoint 测试
// ============================================================================
class TestHealthEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testEmptyEndpoint();
    void testAddAndRemoveIndicator();
    void testIndicatorNames();
    void testReadinessAllUp();
    void testReadinessWithDown();
    void testLivenessOnlyCritical();
    void testLivenessSkipNonCritical();
    void testLivenessWithCriticalDown();
    void testClear();
};

void TestHealthEndpoint::testEmptyEndpoint() {
    HealthEndpoint endpoint;
    // 空端点应返回 UP
    HealthReport report = endpoint.check();
    QCOMPARE(report.overall, HealthStatus::UP);
    QVERIFY(report.details.empty());
}

void TestHealthEndpoint::testAddAndRemoveIndicator() {
    HealthEndpoint endpoint;
    auto indicator = std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "db");

    endpoint.addIndicator(indicator);
    auto names = endpoint.indicatorNames();
    QCOMPARE(names.size(), static_cast<std::size_t>(1));
    QCOMPARE(names[0], std::string("db"));

    endpoint.removeIndicator("db");
    names = endpoint.indicatorNames();
    QCOMPARE(names.size(), static_cast<std::size_t>(0));
}

void TestHealthEndpoint::testIndicatorNames() {
    HealthEndpoint endpoint;
    endpoint.addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "db"));
    endpoint.addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return true; }, "mq"));
    endpoint.addIndicator(std::make_shared<NetworkHealthIndicator>(
        []() { return true; }, "network"));

    auto names = endpoint.indicatorNames();
    QCOMPARE(names.size(), static_cast<std::size_t>(3));
    QVERIFY(std::find(names.begin(), names.end(), std::string("db")) != names.end());
    QVERIFY(std::find(names.begin(), names.end(), std::string("mq")) != names.end());
    QVERIFY(std::find(names.begin(), names.end(), std::string("network")) != names.end());
}

void TestHealthEndpoint::testReadinessAllUp() {
    HealthEndpoint endpoint;
    endpoint.addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "db"));
    endpoint.addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return true; }, "mq"));

    HealthReport report = endpoint.readiness();
    QCOMPARE(report.overall, HealthStatus::UP);
    QCOMPARE(report.details.size(), static_cast<std::size_t>(2));
    QVERIFY(report.totalTime.count() >= 0);
}

void TestHealthEndpoint::testReadinessWithDown() {
    HealthEndpoint endpoint;
    endpoint.addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "db"));
    endpoint.addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return false; }, "bad_db"));

    HealthReport report = endpoint.readiness();
    QCOMPARE(report.overall, HealthStatus::DOWN);
    QCOMPARE(report.details.size(), static_cast<std::size_t>(2));
    QVERIFY(report.totalTime.count() >= 0);
}

void TestHealthEndpoint::testLivenessOnlyCritical() {
    HealthEndpoint endpoint;
    endpoint.addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "db"));
    endpoint.addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return true; }, "mq"));

    // liveness 只检查关键依赖 (db), 不检查 mq
    HealthReport report = endpoint.liveness();
    QCOMPARE(report.overall, HealthStatus::UP);
    QCOMPARE(report.details.size(), static_cast<std::size_t>(1));
    QCOMPARE(report.details[0].name, std::string("db"));
}

void TestHealthEndpoint::testLivenessSkipNonCritical() {
    HealthEndpoint endpoint;
    endpoint.addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return true; }, "mq"));
    endpoint.addIndicator(std::make_shared<NetworkHealthIndicator>(
        []() { return true; }, "network"));

    // liveness: 没有关键依赖 → 返回 UP
    HealthReport report = endpoint.liveness();
    QCOMPARE(report.overall, HealthStatus::UP);
    QVERIFY(report.details.empty());
}

void TestHealthEndpoint::testLivenessWithCriticalDown() {
    HealthEndpoint endpoint;
    // Critical indicator returns DOWN
    endpoint.addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return false; }, "db"));
    // Non-critical indicator returns UP
    endpoint.addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return true; }, "mq"));

    HealthReport report = endpoint.liveness();
    QCOMPARE(report.overall, HealthStatus::DOWN);
    // liveness only includes critical indicators
    QCOMPARE(report.details.size(), static_cast<std::size_t>(1));
    QCOMPARE(report.details[0].name, std::string("db"));
}

void TestHealthEndpoint::testClear() {
    HealthEndpoint endpoint;
    endpoint.addIndicator(std::make_shared<DatabaseHealthIndicator>(
        []() { return true; }, "db"));
    endpoint.addIndicator(std::make_shared<MqHealthIndicator>(
        []() { return true; }, "mq"));

    endpoint.clear();
    auto names = endpoint.indicatorNames();
    QCOMPARE(names.size(), static_cast<std::size_t>(0));

    // 清空后 check() 应返回 UP
    HealthReport report = endpoint.check();
    QCOMPARE(report.overall, HealthStatus::UP);
    QVERIFY(report.details.empty());
}

// ============================================================================
// MqHealthIndicator 测试
// ============================================================================
class TestMqHealthIndicator : public QObject {
    Q_OBJECT
private slots:
    void testUp();
    void testDown();
    void testNonCritical();
};

void TestMqHealthIndicator::testUp() {
    MqHealthIndicator mq([]() { return true; }, "rabbitmq");
    HealthDetail detail = mq.check();
    QCOMPARE(detail.name, std::string("rabbitmq"));
    QCOMPARE(detail.status, HealthStatus::UP);
    QVERIFY(detail.message.find("healthy") != std::string::npos);
}

void TestMqHealthIndicator::testDown() {
    MqHealthIndicator mq([]() { return false; }, "kafka");
    HealthDetail detail = mq.check();
    QCOMPARE(detail.status, HealthStatus::DOWN);
    QVERIFY(detail.message.find("failed") != std::string::npos);
}

void TestMqHealthIndicator::testNonCritical() {
    MqHealthIndicator mq([]() { return true; });
    QVERIFY(!mq.isCritical());
}

// ============================================================================
// NetworkHealthIndicator 测试
// ============================================================================
class TestNetworkHealthIndicator : public QObject {
    Q_OBJECT
private slots:
    void testUp();
    void testDown();
    void testNonCritical();
};

void TestNetworkHealthIndicator::testUp() {
    NetworkHealthIndicator net([]() { return true; }, "api_server");
    HealthDetail detail = net.check();
    QCOMPARE(detail.status, HealthStatus::UP);
}

void TestNetworkHealthIndicator::testDown() {
    NetworkHealthIndicator net([]() { return false; });
    HealthDetail detail = net.check();
    QCOMPARE(detail.status, HealthStatus::DOWN);
}

void TestNetworkHealthIndicator::testNonCritical() {
    NetworkHealthIndicator net([]() { return true; });
    QVERIFY(!net.isCritical());
}

// ============================================================================
// ResourcePoolHealthIndicator 测试
// ============================================================================
class TestResourcePoolHealthIndicator : public QObject {
    Q_OBJECT
private slots:
    void testBelowThreshold();
    void testAboveThreshold();
    void testAtThreshold();
    void testNonCritical();
};

void TestResourcePoolHealthIndicator::testBelowThreshold() {
    ResourcePoolHealthIndicator pool("ThreadPool", []() { return 0.5; }, 0.9);
    HealthDetail detail = pool.check();
    QCOMPARE(detail.status, HealthStatus::UP);
    QVERIFY(detail.message.find("50%") != std::string::npos);
}

void TestResourcePoolHealthIndicator::testAboveThreshold() {
    ResourcePoolHealthIndicator pool("ThreadPool", []() { return 0.95; }, 0.9);
    HealthDetail detail = pool.check();
    QCOMPARE(detail.status, HealthStatus::DOWN);
    QVERIFY(detail.message.find("exceeds") != std::string::npos);
}

void TestResourcePoolHealthIndicator::testAtThreshold() {
    ResourcePoolHealthIndicator pool("ThreadPool", []() { return 0.9; }, 0.9);
    HealthDetail detail = pool.check();
    // 等于阈值视为 DOWN
    QCOMPARE(detail.status, HealthStatus::DOWN);
}

void TestResourcePoolHealthIndicator::testNonCritical() {
    ResourcePoolHealthIndicator pool("ThreadPool", []() { return 0.5; });
    QVERIFY(!pool.isCritical());
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);
    int status = 0;
    {
        TestHealthStatus tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestHealthDetail tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestHealthReport tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestCustomIndicator tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestHealthEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestMqHealthIndicator tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestNetworkHealthIndicator tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestResourcePoolHealthIndicator tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
#include "test_health.moc"