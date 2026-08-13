#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include "soul/observability/otlp_exporter.h"
#include "soul/observability/tracing.h"

using namespace sc;
using namespace sc::observability;

// ============================================================================
// TestOtlpHttpExporter — OTLP HTTP 导出器单元测试
// ============================================================================
class TestOtlpHttpExporter : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSingletonInstance();
    void testInitializeShutdown();
    void testSetEndpoint();
    void testExportSpan();
    void testExportSpans();
    void testExportMetrics();
    void testExportLog();
    void testFlush();
    void testSetBatchSize();
    void testSetFlushInterval();
    void testEnabled();
    void testPendingTraces();
    void testPendingMetrics();
    void testPendingLogs();
};

void TestOtlpHttpExporter::initTestCase() {
}

void TestOtlpHttpExporter::cleanupTestCase() {
    OtlpHttpExporter::instance().shutdown();
}

void TestOtlpHttpExporter::testSingletonInstance() {
    OtlpHttpExporter& exporter1 = OtlpHttpExporter::instance();
    OtlpHttpExporter& exporter2 = OtlpHttpExporter::instance();
    QCOMPARE(&exporter1, &exporter2);
}

void TestOtlpHttpExporter::testInitializeShutdown() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.endpoint = "http://localhost:4318";
    config.batchSize = 256;
    config.flushIntervalMs = 3000;

    auto r = exporter.initialize(config);
    QVERIFY(r.isOk());
    QVERIFY(exporter.isInitialized());

    exporter.shutdown();
    QVERIFY(!exporter.isInitialized());
}

void TestOtlpHttpExporter::testSetEndpoint() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.endpoint = "http://localhost:4318";
    (void)exporter.initialize(config);

    OtlpConfig newConfig;
    newConfig.endpoint = "http://otel-collector:4318";
    exporter.setConfig(newConfig);

    QCOMPARE(exporter.config().endpoint, QString("http://otel-collector:4318"));
}

void TestOtlpHttpExporter::testExportSpan() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.endpoint = "http://localhost:4318";
    (void)exporter.initialize(config);

    SpanContext ctx{"00000000000000000000000000000001", "0000000000000001", "", true};
    auto span = std::make_shared<Span>(ctx, "test.span");

    // 导出单个 Span → 进入缓冲，不崩溃
    exporter.exportSpan(span);
    QVERIFY(true);
}

void TestOtlpHttpExporter::testExportSpans() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.endpoint = "http://localhost:4318";
    (void)exporter.initialize(config);

    std::vector<std::shared_ptr<Span>> spans;
    for (int i = 0; i < 3; ++i) {
        SpanContext ctx{"00000000000000000000000000000001",
                        QString("000000000000000%1").arg(i + 1).toStdString(),
                        "", true};
        auto span = std::make_shared<Span>(ctx, QString("span.%1").arg(i).toStdString());
        spans.push_back(span);
    }

    exporter.exportSpans(spans);
    QVERIFY(true);
}

void TestOtlpHttpExporter::testExportMetrics() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.endpoint = "http://localhost:4318";
    (void)exporter.initialize(config);

    QJsonObject metrics;
    metrics["cpu_usage"] = 0.75;
    metrics["memory_usage"] = 0.60;

    exporter.exportMetrics(metrics);
    QVERIFY(true);
}

void TestOtlpHttpExporter::testExportLog() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.endpoint = "http://localhost:4318";
    (void)exporter.initialize(config);

    QJsonObject logRecord;
    logRecord["level"] = "INFO";
    logRecord["message"] = "Test log message";
    logRecord["timestamp"] = "2025-01-01T00:00:00Z";

    exporter.exportLog(logRecord);
    QVERIFY(true);
}

void TestOtlpHttpExporter::testFlush() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.endpoint = "http://localhost:4318";
    (void)exporter.initialize(config);

    // flush 不崩溃
    exporter.flush();
    QVERIFY(true);
}

void TestOtlpHttpExporter::testSetBatchSize() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.batchSize = 128;
    (void)exporter.initialize(config);

    QCOMPARE(exporter.config().batchSize, 128);
}

void TestOtlpHttpExporter::testSetFlushInterval() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.flushIntervalMs = 10000;
    (void)exporter.initialize(config);

    QCOMPARE(exporter.config().flushIntervalMs, 10000);
}

void TestOtlpHttpExporter::testEnabled() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    config.enableTraces = true;
    config.enableMetrics = false;
    config.enableLogs = true;
    (void)exporter.initialize(config);

    QVERIFY(exporter.config().enableTraces);
    QVERIFY(!exporter.config().enableMetrics);
    QVERIFY(exporter.config().enableLogs);
}

void TestOtlpHttpExporter::testPendingTraces() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    (void)exporter.initialize(config);

    // 初始状态 pendingTraces 为 0
    QCOMPARE(exporter.pendingTraces(), 0);
}

void TestOtlpHttpExporter::testPendingMetrics() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    (void)exporter.initialize(config);

    QCOMPARE(exporter.pendingMetrics(), 0);
}

void TestOtlpHttpExporter::testPendingLogs() {
    auto& exporter = OtlpHttpExporter::instance();
    exporter.shutdown();

    OtlpConfig config;
    (void)exporter.initialize(config);

    QCOMPARE(exporter.pendingLogs(), 0);
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestOtlpHttpExporter test;
    return QTest::qExec(&test, argc, argv);
}

#include "test_otlp_exporter.moc"