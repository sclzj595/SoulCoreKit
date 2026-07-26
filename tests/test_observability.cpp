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
// TraceContext 测试
// ============================================================================
class TestTraceContext : public QObject {
    Q_OBJECT
private slots:
    void testGenerateTraceId();
    void testGenerateSpanId();
    void testToTraceParent();
    void testFromTraceParent();
    void testFromInvalidTraceParent();
    void testIsValid();
};

void TestTraceContext::testGenerateTraceId() {
    std::string id = TraceContext::generateTraceId();
    QCOMPARE(id.size(), static_cast<std::size_t>(32));  // 16 字节 = 32 字符十六进制

    // 两次生成的 ID 应不同
    std::string id2 = TraceContext::generateTraceId();
    QVERIFY(id != id2);
}

void TestTraceContext::testGenerateSpanId() {
    std::string id = TraceContext::generateSpanId();
    QCOMPARE(id.size(), static_cast<std::size_t>(16));  // 8 字节 = 16 字符十六进制

    std::string id2 = TraceContext::generateSpanId();
    QVERIFY(id != id2);
}

void TestTraceContext::testToTraceParent() {
    TraceContext ctx;
    ctx.traceId = "4bf92f3577b34da6a3ce929d0e0e4736";
    ctx.spanId  = "00f067aa0ba902b7";
    std::string tp = ctx.toTraceParent();
    QCOMPARE(QString::fromStdString(tp),
             QString("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"));
}

void TestTraceContext::testFromTraceParent() {
    std::string header = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    TraceContext ctx = TraceContext::fromTraceParent(header);
    QVERIFY(ctx.isValid());
    QCOMPARE(ctx.traceId, std::string("4bf92f3577b34da6a3ce929d0e0e4736"));
    QCOMPARE(ctx.spanId, std::string("00f067aa0ba902b7"));
}

void TestTraceContext::testFromInvalidTraceParent() {
    QVERIFY(!TraceContext::fromTraceParent("").isValid());
    QVERIFY(!TraceContext::fromTraceParent("invalid").isValid());
    QVERIFY(!TraceContext::fromTraceParent("01-abc-abc-01").isValid());  // version 错误
}

void TestTraceContext::testIsValid() {
    TraceContext empty;
    QVERIFY(!empty.isValid());

    TraceContext valid;
    valid.traceId = "abc";
    valid.spanId  = "def";
    QVERIFY(valid.isValid());
}

// ============================================================================
// Span/Tracer 测试
// ============================================================================
class TestTracing : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() { Tracer::instance().clear(); }
    void testStartRootSpan();
    void testStartChildSpan();
    void testSpanAttributes();
    void testSpanEvents();
    void testSpanStatus();
    void testSpanEnd();
    void testSpanGuard();
    void testTracerDisabled();
    void testEndedSpans();
};

void TestTracing::testStartRootSpan() {
    Tracer::instance().clear();
    Tracer::instance().setEnabled(true);

    auto span = Tracer::instance().startSpan("root_operation");
    QVERIFY(span != nullptr);
    QCOMPARE(span->name(), std::string("root_operation"));
    QVERIFY(span->context().isValid());
    QVERIFY(span->context().parentSpanId.empty());  // 根 Span 无父
}

void TestTracing::testStartChildSpan() {
    Tracer::instance().clear();
    Tracer::instance().setEnabled(true);

    auto parent = Tracer::instance().startSpan("parent");
    auto child  = Tracer::instance().startSpan("child", *parent);

    QVERIFY(child != nullptr);
    // 子 Span 继承父 traceId
    QCOMPARE(child->context().traceId, parent->context().traceId);
    // 子 Span 的 parentSpanId 等于父 spanId
    QCOMPARE(child->context().parentSpanId, parent->context().spanId);
    // 子 Span 有自己的 spanId
    QVERIFY(child->context().spanId != parent->context().spanId);
}

void TestTracing::testSpanAttributes() {
    Tracer::instance().clear();
    auto span = Tracer::instance().startSpan("attr_test");

    span->setAttribute("string_attr", "value");
    span->setAttribute("numeric_attr", 42.5);
    span->setAttribute("bool_attr", true);

    auto strAttrs = span->stringAttributes();
    QCOMPARE(strAttrs["string_attr"], std::string("value"));
    QCOMPARE(strAttrs["bool_attr"], std::string("true"));

    auto numAttrs = span->numericAttributes();
    QCOMPARE(numAttrs["numeric_attr"], 42.5);
}

void TestTracing::testSpanEvents() {
    Tracer::instance().clear();
    auto span = Tracer::instance().startSpan("event_test");

    span->addEvent("simple_event");
    span->addEvent("attributed_event", {{"key1", "value1"}});

    auto events = span->events();
    QCOMPARE(events.size(), static_cast<std::size_t>(2));
    QCOMPARE(events[0].name, std::string("simple_event"));
    QCOMPARE(events[1].name, std::string("attributed_event"));
    QCOMPARE(events[1].attributes.at("key1"), std::string("value1"));
}

void TestTracing::testSpanStatus() {
    Tracer::instance().clear();
    auto span = Tracer::instance().startSpan("status_test");

    QCOMPARE(span->status(), SpanStatus::Unset);
    span->setStatus(SpanStatus::Ok);
    QCOMPARE(span->status(), SpanStatus::Ok);
    span->setStatus(SpanStatus::Error, "Database connection failed");
    QCOMPARE(span->status(), SpanStatus::Error);
    QCOMPARE(span->statusDescription(), std::string("Database connection failed"));
}

void TestTracing::testSpanEnd() {
    Tracer::instance().clear();
    auto span = Tracer::instance().startSpan("end_test");

    QVERIFY(!span->isEnded());
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    span->end();
    QVERIFY(span->isEnded());
    QVERIFY(span->duration().count() >= 10);

    // 重复 end 应无副作用
    span->end();
    QVERIFY(span->isEnded());
}

void TestTracing::testSpanGuard() {
    Tracer::instance().clear();
    {
        SpanGuard guard(Tracer::instance().startSpan("guard_test"));
        guard->setAttribute("test", "value");
        QVERIFY(!guard->isEnded());
        // 离开作用域时自动 end
    }
    auto ended = Tracer::instance().endedSpans();
    QCOMPARE(ended.size(), static_cast<std::size_t>(1));
    QVERIFY(ended[0]->isEnded());
    QCOMPARE(ended[0]->name(), std::string("guard_test"));
}

void TestTracing::testTracerDisabled() {
    Tracer::instance().clear();
    Tracer::instance().setEnabled(false);

    auto span = Tracer::instance().startSpan("should_be_null");
    QVERIFY(span == nullptr);

    Tracer::instance().setEnabled(true);
}

void TestTracing::testEndedSpans() {
    Tracer::instance().clear();
    Tracer::instance().setEnabled(true);

    auto s1 = Tracer::instance().startSpan("s1");
    auto s2 = Tracer::instance().startSpan("s2");
    s1->end();
    // s2 未结束

    auto ended = Tracer::instance().endedSpans();
    QCOMPARE(ended.size(), static_cast<std::size_t>(1));
    QCOMPARE(ended[0]->name(), std::string("s1"));

    s2->end();
    ended = Tracer::instance().endedSpans();
    QCOMPARE(ended.size(), static_cast<std::size_t>(2));
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
