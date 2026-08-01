#include <QTest>
#include <QTemporaryFile>
#include <QString>
#include <QFile>
#include <QTextStream>
#include <sstream>
#include <chrono>
#include <thread>

#include "soul/observability/metrics.h"
#include "soul/observability/tracing.h"
#include "soul/observability/json_sink.h"
#include "soul/logging/log_record.h"
#include "soul/logging/log_level.h"

using namespace sc;
using namespace sc::observability;

// ============================================================================
// Counter 测试
// ============================================================================
class TestCounter : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { MetricsRegistry::instance().clear(); }
    void testIncrement();
    void testIncrementByValue();
    void testNegativeIncrementIgnored();
    void testLabeledIncrement();
    void testRegistryReturnsSameInstance();
};

void TestCounter::testIncrement() {
    MetricsRegistry::instance().clear();
    auto& c = MetricsRegistry::instance().counter("test_counter_1", "Test counter");
    QCOMPARE(c.value(), 0.0);
    c.increment();
    c.increment();
    QCOMPARE(c.value(), 2.0);
}

void TestCounter::testIncrementByValue() {
    MetricsRegistry::instance().clear();
    auto& c = MetricsRegistry::instance().counter("test_counter_2", "Test counter");
    c.increment(5.5);
    c.increment(4.5);
    QCOMPARE(c.value(), 10.0);
}

void TestCounter::testNegativeIncrementIgnored() {
    MetricsRegistry::instance().clear();
    auto& c = MetricsRegistry::instance().counter("test_counter_3", "Test counter");
    c.increment(10);
    c.increment(-5);  // 应被忽略
    QCOMPARE(c.value(), 10.0);
}

void TestCounter::testLabeledIncrement() {
    MetricsRegistry::instance().clear();
    auto& c = MetricsRegistry::instance().counter("test_counter_4", "Test counter");
    Labels getLabels{{"method", "GET"}};
    Labels postLabels{{"method", "POST"}};
    c.increment(getLabels);
    c.increment(getLabels);
    c.increment(postLabels);

    auto labeled = c.labeledValues();
    QCOMPARE(labeled.size(), static_cast<std::size_t>(2));
    QCOMPARE(labeled[getLabels], 2.0);
    QCOMPARE(labeled[postLabels], 1.0);
}

void TestCounter::testRegistryReturnsSameInstance() {
    MetricsRegistry::instance().clear();
    auto& c1 = MetricsRegistry::instance().counter("same_counter", "First");
    auto& c2 = MetricsRegistry::instance().counter("same_counter", "Second");
    QVERIFY(&c1 == &c2);
}

// ============================================================================
// Gauge 测试
// ============================================================================
class TestGauge : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { MetricsRegistry::instance().clear(); }
    void testSet();
    void testIncrementDecrement();
};

void TestGauge::testSet() {
    MetricsRegistry::instance().clear();
    auto& g = MetricsRegistry::instance().gauge("test_gauge_1", "Test gauge");
    g.set(42.0);
    QCOMPARE(g.value(), 42.0);
    g.set(100.0);
    QCOMPARE(g.value(), 100.0);
}

void TestGauge::testIncrementDecrement() {
    MetricsRegistry::instance().clear();
    auto& g = MetricsRegistry::instance().gauge("test_gauge_2", "Test gauge");
    g.set(50.0);
    g.increment(10.0);
    QCOMPARE(g.value(), 60.0);
    g.decrement(5.0);
    QCOMPARE(g.value(), 55.0);
}

// ============================================================================
// Histogram 测试
// ============================================================================
class TestHistogram : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { MetricsRegistry::instance().clear(); }
    void testObserve();
    void testSnapshot();
    void testQuantile();
    void testDefaultBuckets();
};

void TestHistogram::testObserve() {
    MetricsRegistry::instance().clear();
    auto& h = MetricsRegistry::instance().histogram(
        "test_hist_1", "Test histogram", HistogramBuckets::defaultLatency());
    h.observe(10);
    h.observe(20);
    h.observe(30);
    auto snap = h.snapshot();
    QCOMPARE(snap.count, static_cast<std::uint64_t>(3));
    QCOMPARE(snap.sum, 60.0);
    QCOMPARE(snap.min, 10.0);
    QCOMPARE(snap.max, 30.0);
}

void TestHistogram::testSnapshot() {
    MetricsRegistry::instance().clear();
    auto& h = MetricsRegistry::instance().histogram(
        "test_hist_2", "Test histogram", HistogramBuckets::defaultLatency());
    h.observe(5);
    h.observe(50);
    h.observe(500);
    auto snap = h.snapshot();
    QCOMPARE(snap.count, static_cast<std::uint64_t>(3));
    QVERIFY(snap.bucketCounts.size() > 0);
    // 桶计数总和应等于总观测数
    std::uint64_t bucketSum = 0;
    for (auto c : snap.bucketCounts) bucketSum += c;
    QCOMPARE(bucketSum, static_cast<std::uint64_t>(3));
}

void TestHistogram::testQuantile() {
    MetricsRegistry::instance().clear();
    auto& h = MetricsRegistry::instance().histogram(
        "test_hist_3", "Test histogram", HistogramBuckets::defaultLatency());
    // 观测 100 个值：1, 2, 3, ..., 100
    for (int i = 1; i <= 100; ++i) {
        h.observe(static_cast<double>(i));
    }
    auto snap = h.snapshot();
    // 中位数（p50）应接近 50
    double p50 = snap.quantile(0.5);
    QVERIFY(p50 >= 40.0 && p50 <= 60.0);
    // p99 应接近 99
    double p99 = snap.quantile(0.99);
    QVERIFY(p99 >= 90.0 && p99 <= 100.0);
}

void TestHistogram::testDefaultBuckets() {
    auto latency = HistogramBuckets::defaultLatency();
    QVERIFY(latency.boundaries.size() > 0);
    QVERIFY(latency.bucketCount() == latency.boundaries.size() + 1);

    auto size = HistogramBuckets::defaultSize();
    QVERIFY(size.boundaries.size() > 0);
}

// ============================================================================
// SpanContext / Tracer 测试 (v1.9.3 简化版, 核心 W3C Trace Context)
// ============================================================================
class TestTraceContext : public QObject {
    Q_OBJECT
private slots:
    void testGenerateTraceId();
    void testGenerateSpanId();
    void testFormatTraceparent();
    void testParseTraceparent();
    void testParseInvalidTraceparent();
    void testSpanContextValid();
    void testExtractFromHeaders();
    void testInjectToHeaders();
};

void TestTraceContext::testGenerateTraceId() {
    std::string id = Tracer::generateTraceId();
    QCOMPARE(id.size(), static_cast<std::size_t>(32));

    std::string id2 = Tracer::generateTraceId();
    QVERIFY(id != id2);
}

void TestTraceContext::testGenerateSpanId() {
    std::string id = Tracer::generateSpanId();
    QCOMPARE(id.size(), static_cast<std::size_t>(16));

    std::string id2 = Tracer::generateSpanId();
    QVERIFY(id != id2);
}

void TestTraceContext::testFormatTraceparent() {
    SpanContext ctx;
    ctx.traceId = "4bf92f3577b34da6a3ce929d0e0e4736";
    ctx.spanId  = "00f067aa0ba902b7";
    ctx.sampled = true;
    std::string tp = Tracer::formatTraceparent(ctx);
    QCOMPARE(QString::fromStdString(tp),
             QString("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"));
}

void TestTraceContext::testParseTraceparent() {
    std::string header = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    SpanContext ctx = Tracer::parseTraceparent(header);
    QVERIFY(ctx.isValid());
    QCOMPARE(ctx.traceId, std::string("4bf92f3577b34da6a3ce929d0e0e4736"));
    QCOMPARE(ctx.spanId, std::string("00f067aa0ba902b7"));
    QVERIFY(ctx.sampled);
}

void TestTraceContext::testParseInvalidTraceparent() {
    QVERIFY(!Tracer::parseTraceparent("").isValid());
    QVERIFY(!Tracer::parseTraceparent("invalid").isValid());
    QVERIFY(!Tracer::parseTraceparent("01-abc-abc-01").isValid());
}

void TestTraceContext::testSpanContextValid() {
    SpanContext empty;
    QVERIFY(!empty.isValid());

    SpanContext valid;
    valid.traceId = "abc";
    valid.spanId  = "def";
    QVERIFY(valid.isValid());
}

void TestTraceContext::testExtractFromHeaders() {
    QMap<QString, QString> headers;
    headers["traceparent"] = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    SpanContext ctx = Tracer::extractFromHeaders(headers);
    QVERIFY(ctx.isValid());
    QCOMPARE(ctx.traceId, std::string("4bf92f3577b34da6a3ce929d0e0e4736"));

    QMap<QString, QString> emptyHeaders;
    SpanContext emptyCtx = Tracer::extractFromHeaders(emptyHeaders);
    QVERIFY(!emptyCtx.isValid());
}

void TestTraceContext::testInjectToHeaders() {
    auto span = Tracer::instance().startSpan("test");

    QMap<QString, QString> headers;
    Tracer::injectToHeaders(span, headers);
    QVERIFY(headers.contains("traceparent"));

    // traceparent 格式: 00-traceId-spanId-01
    std::string tp = headers["traceparent"].toStdString();
    QVERIFY(tp.find("00-") == 0);
    QVERIFY(tp.rfind("-01") == tp.size() - 3);
    // traceId 32 hex + spanId 16 hex = 48 hex chars between dashes
    auto firstDash = tp.find('-');
    auto secondDash = tp.find('-', firstDash + 1);
    auto thirdDash = tp.find('-', secondDash + 1);
    QVERIFY(secondDash - firstDash - 1 == 32);  // traceId length
    QVERIFY(thirdDash - secondDash - 1 == 16);  // spanId length
}

// ============================================================================
// Span / Tracer 测试 (v1.9.3 简化版)
// ============================================================================
class TestTracing : public QObject {
    Q_OBJECT
private slots:
    void testStartRootSpan();
    void testStartChildSpan();
    void testSpanEnd();
    void testSpanTags();
    void testSpanEvents();
    void testSpanStatus();
};

void TestTracing::testStartRootSpan() {
    auto span = Tracer::instance().startSpan("root_operation");
    QVERIFY(span != nullptr);
    QCOMPARE(span->name(), std::string("root_operation"));
    QVERIFY(span->context().isValid());
    QVERIFY(span->context().parentSpanId.empty());
}

void TestTracing::testStartChildSpan() {
    auto parent = Tracer::instance().startSpan("parent");
    auto child  = Tracer::instance().startSpan("child", parent->context());

    QVERIFY(child != nullptr);
    QCOMPARE(child->context().traceId, parent->context().traceId);
    QCOMPARE(child->context().parentSpanId, parent->context().spanId);
    QVERIFY(child->context().spanId != parent->context().spanId);
}

void TestTracing::testSpanEnd() {
    auto span = Tracer::instance().startSpan("end_test");
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    span->end();
    QVERIFY(span->durationMs() >= 10);
}

void TestTracing::testSpanTags() {
    auto span = Tracer::instance().startSpan("tag_test");
    span->setTag("http.method", "GET");
    span->setTag("http.status_code", "200");

    const auto& tags = span->getTags();
    QCOMPARE(tags.size(), static_cast<std::size_t>(2));
    QCOMPARE(tags.at("http.method"), std::string("GET"));
    QCOMPARE(tags.at("http.status_code"), std::string("200"));
}

void TestTracing::testSpanEvents() {
    auto span = Tracer::instance().startSpan("event_test");
    span->addEvent("cache_miss");
    span->addEvent("db_query", {{"table", "users"}, {"duration_ms", "15"}});

    const auto& events = span->getEvents();
    QCOMPARE(events.size(), static_cast<std::size_t>(2));
    QCOMPARE(events[0].name, std::string("cache_miss"));
    QCOMPARE(events[1].name, std::string("db_query"));
    QCOMPARE(events[1].attributes.at("table"), std::string("users"));
    QCOMPARE(events[1].attributes.at("duration_ms"), std::string("15"));
}

void TestTracing::testSpanStatus() {
    auto span = Tracer::instance().startSpan("status_test");
    span->setStatus(true, "OK");
    QVERIFY(span->isOk());
    QCOMPARE(span->statusDescription(), std::string("OK"));

    span->setStatus(false, "Database connection failed");
    QVERIFY(!span->isOk());
    QCOMPARE(span->statusDescription(), std::string("Database connection failed"));
}

// ============================================================================
// JsonSink 测试
// ============================================================================
class TestJsonSink : public QObject {
    Q_OBJECT
private slots:
    void testSerializeBasicRecord();
    void testSerializeWithSpecialChars();
    void testStreamSink();
    void testFileSink();
};

void TestJsonSink::testSerializeBasicRecord() {
    std::ostringstream oss;
    JsonSink sink(oss);

    LogRecord record;
    record.level      = LogLevel::Info;
    record.module     = "network";
    record.operation  = "connect";
    record.message    = "Connected to server";
    record.timestamp  = "2026-07-26T10:30:45.123Z";
    record.file       = "client.cpp";
    record.line       = 42;
    record.threadId   = "0x7f8a";
    record.processId  = "12345";

    sink.log(record);
    sink.flush();

    std::string output = oss.str();
    QVERIFY(output.find("\"level\":\"INFO\"") != std::string::npos);
    QVERIFY(output.find("\"module\":\"network\"") != std::string::npos);
    QVERIFY(output.find("\"operation\":\"connect\"") != std::string::npos);
    QVERIFY(output.find("\"message\":\"Connected to server\"") != std::string::npos);
    QVERIFY(output.find("\"line\":42") != std::string::npos);
    QVERIFY(output.find("\"thread_id\":\"0x7f8a\"") != std::string::npos);
    QVERIFY(output.find("\"process_id\":\"12345\"") != std::string::npos);
}

void TestJsonSink::testSerializeWithSpecialChars() {
    std::ostringstream oss;
    JsonSink sink(oss);

    LogRecord record;
    record.level     = LogLevel::Error;
    record.message   = "Error: \"path\" contains \\ and \n newline";
    record.timestamp = "2026-07-26T10:30:45.123Z";

    sink.log(record);
    sink.flush();

    std::string output = oss.str();
    // 验证引号被转义
    QVERIFY(output.find("\\\"path\\\"") != std::string::npos);
    // 验证反斜杠被转义
    QVERIFY(output.find("\\\\") != std::string::npos);
    // 验证换行被转义
    QVERIFY(output.find("\\n") != std::string::npos);
}

void TestJsonSink::testStreamSink() {
    std::ostringstream oss;
    JsonSink sink(oss);

    LogRecord record;
    record.level    = LogLevel::Warn;
    record.message  = "Warning message";
    record.timestamp = "2026-07-26T10:30:45.123Z";

    sink.log(record);
    sink.log(record);  // 写入两条
    sink.flush();

    std::string output = oss.str();
    // 每条日志一行
    std::size_t newlineCount = std::count(output.begin(), output.end(), '\n');
    QCOMPARE(newlineCount, static_cast<std::size_t>(2));
}

void TestJsonSink::testFileSink() {
    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(true);
    QVERIFY(tmpFile.open());
    QString filePath = tmpFile.fileName();
    tmpFile.close();

    {
        JsonSink sink(filePath);
        LogRecord record;
        record.level    = LogLevel::Info;
        record.message  = "File sink test";
        record.timestamp = "2026-07-26T10:30:45.123Z";
        sink.log(record);
        sink.flush();
    }

    QFile file(filePath);
    QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
    QString content = QTextStream(&file).readAll();
    QVERIFY(content.contains("\"message\":\"File sink test\""));
    QVERIFY(content.contains("\"level\":\"INFO\""));
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);
    int status = 0;
    {
        TestCounter tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestGauge tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestHistogram tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestTraceContext tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestTracing tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestJsonSink tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
#include "test_observability.moc"
