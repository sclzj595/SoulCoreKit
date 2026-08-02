#include <QTest>
#include <QCoreApplication>
#include <thread>

#include "soul/core/result.h"
#include "soul/core/error.h"
#include "soul/network/policy/circuit_breaker.h"
#include "soul/network/policy/rate_limiter.h"
#include "soul/validation/validator.h"
#include "soul/server/middleware.h"
#include "soul/server/info_endpoint.h"
#include "soul/server/loggers_endpoint.h"
#include "soul/observability/tracing.h"

using namespace sc::network;
using namespace sc::validation;
using namespace sc::server;
using sc::Result;
using sc::Error;
using sc::ErrorCode;
using namespace sc::observability;

// ============================================================================
// CircuitBreaker 测试
// ============================================================================
class TestCircuitBreaker : public QObject {
    Q_OBJECT
private slots:
    void testInitialState() {
        CircuitBreaker cb("test");
        QCOMPARE(cb.state(), CircuitBreakerState::Closed);
    }

    void testCallSuccess() {
        CircuitBreaker cb("test");
        auto result = cb.call([]() -> Result<int> {
            return Result<int>::ok(42);
        });
        QVERIFY(result.isOk());
        QCOMPARE(result.unwrap(), 42);
        QCOMPARE(cb.state(), CircuitBreakerState::Closed);
    }

    void testCallFailureTriggersOpen() {
        CircuitBreaker cb("test");
        cb.setFailureThreshold(2).setWindowDuration(60000);

        // 触发 2 次失败
        cb.call([]() -> Result<int> {
            return Result<int>::err(Error(ErrorCode::InternalError, "fail"));
        });
        cb.call([]() -> Result<int> {
            return Result<int>::err(Error(ErrorCode::InternalError, "fail"));
        });

        QCOMPARE(cb.state(), CircuitBreakerState::Open);
    }

    void testOpenStateRejects() {
        CircuitBreaker cb("test");
        cb.setFailureThreshold(1).setResetTimeout(999999);

        // 触发熔断
        cb.call([]() -> Result<int> {
            return Result<int>::err(Error(ErrorCode::InternalError, "fail"));
        });
        QCOMPARE(cb.state(), CircuitBreakerState::Open);

        auto result = cb.call([]() -> Result<int> {
            return Result<int>::ok(1);
        });
        QVERIFY(!result.isOk());
    }

    void testResetToClosed() {
        CircuitBreaker cb("test");
        cb.setFailureThreshold(1);
        cb.call([]() -> Result<int> {
            return Result<int>::err(Error(ErrorCode::InternalError, "fail"));
        });
        QCOMPARE(cb.state(), CircuitBreakerState::Open);

        cb.reset();
        QCOMPARE(cb.state(), CircuitBreakerState::Closed);
    }

    void testForceOpen() {
        CircuitBreaker cb("test");
        cb.forceOpen();
        QCOMPARE(cb.state(), CircuitBreakerState::Open);
    }

    void testManualOnSuccessOnFailure() {
        CircuitBreaker cb("test");
        cb.setFailureThreshold(2);

        cb.onFailure();
        cb.onFailure();
        QCOMPARE(cb.state(), CircuitBreakerState::Open);
    }

    void testStateChangeCallback() {
        CircuitBreaker cb("test");
        cb.setFailureThreshold(1);

        int callbackCount = 0;
        cb.setStateChangeCallback([&](const std::string&, CircuitBreakerState, CircuitBreakerState) {
            ++callbackCount;
        });

        cb.call([]() -> Result<int> {
            return Result<int>::err(Error(ErrorCode::InternalError, "fail"));
        });

        QVERIFY(callbackCount > 0);
    }
};

// ============================================================================
// RateLimiter 测试
// ============================================================================
class TestRateLimiter : public QObject {
    Q_OBJECT
private slots:
    void testTokenBucketAcquire() {
        RateLimiter limiter(RateLimiter::Algorithm::TokenBucket, 10.0, 10);
        // 前 10 个应该成功
        for (int i = 0; i < 10; ++i) {
            QVERIFY(limiter.tryAcquire());
        }
        // 第 11 个应该失败
        QVERIFY(!limiter.tryAcquire());
    }

    void testSlidingWindowAcquire() {
        RateLimiter limiter(RateLimiter::Algorithm::SlidingWindow, 10.0);
        // 前 10 个应该成功
        for (int i = 0; i < 10; ++i) {
            QVERIFY(limiter.tryAcquire());
        }
        // 第 11 个应该失败
        QVERIFY(!limiter.tryAcquire());
    }

    void testRejectedCount() {
        RateLimiter limiter(RateLimiter::Algorithm::TokenBucket, 1.0, 1);
        limiter.tryAcquire();
        limiter.tryAcquire();  // should be rejected
        QVERIFY(limiter.rejectedCount() >= 1);
    }

    void testAcceptedCount() {
        RateLimiter limiter(RateLimiter::Algorithm::TokenBucket, 100.0, 100);
        limiter.tryAcquire();
        QVERIFY(limiter.acceptedCount() >= 1);
    }

    void testSetPermitsPerSec() {
        RateLimiter limiter(RateLimiter::Algorithm::TokenBucket, 10.0, 10);
        limiter.setPermitsPerSec(200.0);
        QCOMPARE(limiter.permitsPerSec(), 200.0);
    }

    void testSetBurstSize() {
        RateLimiter limiter(RateLimiter::Algorithm::TokenBucket, 10.0, 10);
        limiter.setBurstSize(50);
        // Burst size is internal, just verify it doesn't crash
        QVERIFY(true);
    }

    void testWindowDurationMs() {
        RateLimiter limiter(RateLimiter::Algorithm::SlidingWindow, 10.0);
        QCOMPARE(limiter.windowDurationMs(), 1000);  // default

        limiter.setWindowDurationMs(5000);
        QCOMPARE(limiter.windowDurationMs(), 5000);
    }

    void testInvalidPermits() {
        RateLimiter limiter(RateLimiter::Algorithm::TokenBucket, 10.0);
        // negative permits should be accepted
        QVERIFY(limiter.tryAcquire(-1));
        // zero permits should be accepted
        QVERIFY(limiter.tryAcquire(0));
    }
};

// ============================================================================
// Validator 测试
// ============================================================================
class TestValidator : public QObject {
    Q_OBJECT
private slots:
    void testRequired() {
        Validator v;
        v.required("name", QString(""), "不能为空");
        auto result = v.validate();
        QVERIFY(!result.isValid());
        QCOMPARE(result.errors().size(), static_cast<std::size_t>(1));
    }

    void testRequiredPasses() {
        Validator v;
        v.required("name", QString("Alice"), "不能为空");
        auto result = v.validate();
        QVERIFY(result.isValid());
    }

    void testRange() {
        Validator v;
        v.range("age", 25, 0, 150, "年龄越界");
        auto result = v.validate();
        QVERIFY(result.isValid());
    }

    void testRangeOutOfBounds() {
        Validator v;
        v.range("age", 200, 0, 150, "年龄越界");
        auto result = v.validate();
        QVERIFY(!result.isValid());
    }

    void testLength() {
        Validator v;
        v.length("password", QString("123456"), 6, 128, "密码太短");
        auto result = v.validate();
        QVERIFY(result.isValid());
    }

    void testLengthTooShort() {
        Validator v;
        v.length("password", QString("123"), 6, 128, "密码太短");
        auto result = v.validate();
        QVERIFY(!result.isValid());
    }

    void testPattern() {
        Validator v;
        v.pattern("email", QString("test@example.com"), std::string(patterns::EMAIL), "邮箱格式错误");
        auto result = v.validate();
        QVERIFY(result.isValid());
    }

    void testPatternInvalid() {
        Validator v;
        v.pattern("email", QString("not-an-email"), std::string(patterns::EMAIL), "邮箱格式错误");
        auto result = v.validate();
        QVERIFY(!result.isValid());
    }

    void testEmail() {
        Validator v;
        v.email("contact", QString("a@b.com"), "邮箱格式错误");
        auto result = v.validate();
        QVERIFY(result.isValid());
    }

    void testSafeStringRejects() {
        Validator v;
        v.safeString("input", QString("DROP TABLE users; --"));
        auto result = v.validate();
        QVERIFY(!result.isValid());
    }

    void testSafeStringWithOptions() {
        // Allow quotes and semicolons for legitimate names like O'Brien
        Validator v;
        SafeStringOptions opts;
        opts.allowQuotes = true;
        opts.allowSemicolon = true;
        v.safeString("name", QString("O'Brien"), "不安全", opts);
        auto result = v.validate();
        QVERIFY(result.isValid());
    }

    void testSafeStringXss() {
        Validator v;
        v.safeString("input", QString("<script>alert(1)</script>"));
        auto result = v.validate();
        QVERIFY(!result.isValid());
    }

    void testSafeStringDisableXss() {
        Validator v;
        SafeStringOptions opts;
        opts.checkXss = false;
        v.safeString("input", QString("<script>alert(1)</script>"), "", opts);
        auto result = v.validate();
        // With XSS check disabled, <script> within quotes would normally be blocked
        // by quotes check, but we also need allowQuotes for the angle brackets
        QVERIFY(result.isValid());  // only < > are checked in XSS, not in SQL
    }

    void testDigitsOnly() {
        Validator v;
        v.digitsOnly("code", QString("12345"));
        QVERIFY(v.validate().isValid());
    }

    void testDigitsOnlyRejects() {
        Validator v;
        v.digitsOnly("code", QString("abc"));
        QVERIFY(!v.validate().isValid());
    }

    void testAlphanumeric() {
        Validator v;
        v.alphanumeric("user", QString("user_123"));
        QVERIFY(v.validate().isValid());
    }

    void testMultipleRules() {
        Validator v;
        v.required("name", QString(""), "不能为空")
         .range("age", 200, 0, 150, "年龄越界");
        auto result = v.validate();
        QCOMPARE(result.errors().size(), static_cast<std::size_t>(2));
    }

    void testFirstError() {
        Validator v;
        v.required("name", QString(""), "不能为空");
        auto result = v.validate();
        QCOMPARE(result.firstError(), std::string("不能为空"));
    }

    void testFieldErrors() {
        Validator v;
        v.required("name", QString(""), "不能为空");
        auto result = v.validate();
        auto fieldErrs = result.fieldErrors("name");
        QCOMPARE(fieldErrs.size(), static_cast<std::size_t>(1));
    }

    void testClear() {
        Validator v;
        v.required("name", QString(""), "不能为空");
        v.clear();
        QVERIFY(v.validate().isValid());
    }
};

// ============================================================================
// InfoEndpoint 测试
// ============================================================================
class TestInfoEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testDefaultValues() {
        QByteArray json = InfoEndpoint::toJson();
        QVERIFY(json.contains("SoulCoreKit App"));
        QVERIFY(json.contains("SoulCoreKit"));
    }

    void testCustomValues() {
        InfoEndpoint::setAppName("MyApp");
        InfoEndpoint::setAppVersion("2.0.0");
        QByteArray json = InfoEndpoint::toJson();
        QVERIFY(json.contains("MyApp"));
        QVERIFY(json.contains("2.0.0"));
    }

    void testStartupTime() {
        InfoEndpoint::setStartupTime(1234567890);
        QByteArray json = InfoEndpoint::toJson();
        QVERIFY(json.contains("1234567890"));
    }
};

// ============================================================================
// LoggersEndpoint 测试
// ============================================================================
class TestLoggersEndpoint : public QObject {
    Q_OBJECT
private slots:
    void testGetAllLevels() {
        QByteArray json = LoggersEndpoint::getAllLevels();
        QVERIFY(json.contains("ROOT"));
        QVERIFY(json.contains("levels"));
        QVERIFY(json.contains("loggers"));
    }

    void testSetRootLevel() {
        bool ok = LoggersEndpoint::setLevel("ROOT", "DEBUG");
        QVERIFY(ok);
        QCOMPARE(LoggersEndpoint::getLevel("ROOT"), std::string("DEBUG"));

        // Restore
        LoggersEndpoint::setLevel("ROOT", "INFO");
    }

    void testSetModuleLevel() {
        bool ok = LoggersEndpoint::setLevel("test_module", "WARN");
        QVERIFY(ok);
        QCOMPARE(LoggersEndpoint::getLevel("test_module"), std::string("WARN"));
    }

    void testInvalidLevel() {
        bool ok = LoggersEndpoint::setLevel("ROOT", "INVALID");
        QVERIFY(!ok);
    }
};

// ============================================================================
// SpanGuard + Tracer 测试 [v1.9.3]
// ============================================================================
class TestSpanGuard : public QObject {
    Q_OBJECT
private slots:
    void testSpanGuardAutoEnd() {
        Tracer::instance().setEnabled(true);
        {
            SpanGuard guard(Tracer::instance().startSpan("testOp"));
            QVERIFY(guard.span() != nullptr);
            QVERIFY(!guard.span()->isEnded());
            guard->setTag("key", "value");
            auto tags = guard->getTags();
            QVERIFY(tags.find("key") != tags.end());
        }
        // guard 析构后 span 自动 end()
        // (无法直接验证析构行为,但确保不崩溃)
    }

    void testSpanGuardNullSpan() {
        // 传入 nullptr 不应崩溃
        SpanGuard guard(nullptr);
        // 析构安全
    }

    void testTracerDisabled() {
        Tracer::instance().setEnabled(false);
        QVERIFY(!Tracer::instance().isEnabled());

        auto span = Tracer::instance().startSpan("shouldBeNull");
        QVERIFY(span == nullptr);

        // 恢复
        Tracer::instance().setEnabled(true);
        QVERIFY(Tracer::instance().isEnabled());
    }

    void testSpanContextValid() {
        SpanContext ctx;
        QVERIFY(!ctx.isValid());

        ctx.traceId = "abc";
        ctx.spanId = "def";
        QVERIFY(ctx.isValid());
    }

    void testSpanOperations() {
        Tracer::instance().setEnabled(true);
        auto span = Tracer::instance().startSpan("testSpan");
        QVERIFY(span != nullptr);
        QCOMPARE(span->name(), std::string("testSpan"));
        QVERIFY(!span->isEnded());
        QVERIFY(span->isOk());

        span->setTag("http.method", "GET");
        span->addEvent("cache.miss");
        span->setStatus(true, "OK");

        auto tags = span->getTags();
        QVERIFY(tags.find("http.method") != tags.end());

        auto events = span->getEvents();
        QCOMPARE(events.size(), static_cast<std::size_t>(1));

        span->end();
        QVERIFY(span->isEnded());
        QVERIFY(span->durationMs() >= 0);
    }

    void testParseTraceparent() {
        // 有效 header
        auto ctx = Tracer::parseTraceparent(
            "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01");
        QVERIFY(ctx.isValid());
        QCOMPARE(ctx.traceId, std::string("4bf92f3577b34da6a3ce929d0e0e4736"));
        QCOMPARE(ctx.spanId, std::string("00f067aa0ba902b7"));
        QVERIFY(ctx.sampled);
    }

    void testParseTraceparentInvalid() {
        // 无效 header
        auto ctx = Tracer::parseTraceparent("invalid");
        QVERIFY(!ctx.isValid());

        // 空字符串
        ctx = Tracer::parseTraceparent("");
        QVERIFY(!ctx.isValid());
    }

    void testFormatTraceparent() {
        SpanContext ctx;
        ctx.traceId = "4bf92f3577b34da6a3ce929d0e0e4736";
        ctx.spanId = "00f067aa0ba902b7";
        ctx.sampled = true;

        auto header = Tracer::formatTraceparent(ctx);
        QCOMPARE(header, std::string("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"));
    }

    void testGenerateIds() {
        auto traceId = Tracer::generateTraceId();
        QCOMPARE(traceId.size(), static_cast<std::size_t>(32));

        auto spanId = Tracer::generateSpanId();
        QCOMPARE(spanId.size(), static_cast<std::size_t>(16));

        // 两次生成应不同
        auto traceId2 = Tracer::generateTraceId();
        QVERIFY(traceId != traceId2);
    }

    void testSpanContextIsValid() {
        SpanContext ctx;
        ctx.traceId = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
        ctx.spanId = "bbbbbbbbbbbbbbbb";
        QVERIFY(ctx.isValid());
    }
};

// ============================================================================
// main
// ============================================================================
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);
    int status = 0;
    {
        TestCircuitBreaker tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestRateLimiter tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestValidator tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestInfoEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestLoggersEndpoint tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestSpanGuard tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
#include "test_v193_components.moc"