#include <QTest>
#include "soul/core/result.h"

class TestResult : public QObject {
    Q_OBJECT

private slots:
    void testOkResult();
    void testErrorResult();
    void testResultVoid();
    void testResultChaining();
    void testStaticFactoryMethods();
    void testUnwrapOr();
    void testUnwrapOrElse();
    void testMapErr();
    void testOrElse();
    void testOptionalMethods();
};

void TestResult::testOkResult() {
    sc::Result<int> result(42);
    QVERIFY(result.isOk());
    QVERIFY(!result.isErr());
    QCOMPARE(result.unwrap(), 42);
}

void TestResult::testErrorResult() {
    sc::Result<int> result(sc::Error(sc::ErrorCode::NotFound, "Not found"));
    QVERIFY(result.isErr());
    QVERIFY(!result.isOk());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestResult::testResultVoid() {
    sc::Result<void> success;
    QVERIFY(success.isOk());

    sc::Result<void> failure = sc::Error(sc::ErrorCode::InternalError, "Error");
    QVERIFY(failure.isErr());
}

void TestResult::testResultChaining() {
    sc::Result<int> result(10);
    auto mapped = result.map([](int v) { return v * 2; });
    QVERIFY(mapped.isOk());
    QCOMPARE(mapped.unwrap(), 20);
}

void TestResult::testStaticFactoryMethods() {
    auto success = sc::Result<int>::ok(42);
    QVERIFY(success.isOk());
    QCOMPARE(success.unwrap(), 42);

    auto error = sc::Result<int>::err(sc::Error(sc::ErrorCode::NotFound, "Test"));
    QVERIFY(error.isErr());
    QCOMPARE(error.unwrapErr().code(), sc::ErrorCode::NotFound);

    auto voidSuccess = sc::Result<void>::ok();
    QVERIFY(voidSuccess.isOk());

    auto voidError = sc::Result<void>::err(sc::Error(sc::ErrorCode::InternalError, "Void error"));
    QVERIFY(voidError.isErr());
}

void TestResult::testUnwrapOr() {
    sc::Result<int> success(42);
    QCOMPARE(success.unwrapOr(0), 42);

    sc::Result<int> error(sc::Error(sc::ErrorCode::NotFound, "Not found"));
    QCOMPARE(error.unwrapOr(100), 100);
}

void TestResult::testUnwrapOrElse() {
    sc::Result<int> success(42);
    QCOMPARE(success.unwrapOrElse([](const sc::Error&) { return 99; }), 42);

    sc::Result<int> error(sc::Error(sc::ErrorCode::NotFound, "Not found"));
    sc::ErrorCode capturedCode = sc::ErrorCode::Unknown;
    int result = error.unwrapOrElse([&capturedCode](const sc::Error& e) -> int {
        capturedCode = e.code();
        return 100;
    });
    QCOMPARE(capturedCode, sc::ErrorCode::NotFound);
    QCOMPARE(result, 100);
}

void TestResult::testMapErr() {
    sc::Result<int> success(42);
    auto mapped = success.mapErr([](const sc::Error& /*e*/) {
        return sc::Error(sc::ErrorCode::InternalError, "Modified");
    });
    QVERIFY(mapped.isOk());
    QCOMPARE(mapped.unwrap(), 42);

    sc::Result<int> error(sc::Error(sc::ErrorCode::NotFound, "Original"));
    auto errorMapped = error.mapErr([](const sc::Error&) {
        return sc::Error(sc::ErrorCode::InternalError, "Mapped");
    });
    QVERIFY(errorMapped.isErr());
    QCOMPARE(errorMapped.unwrapErr().code(), sc::ErrorCode::InternalError);
}

void TestResult::testOrElse() {
    sc::Result<int> success(42);
    auto result = success.orElse([](const sc::Error&) {
        return sc::Result<int>::ok(99);
    });
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap(), 42);

    sc::Result<int> error(sc::Error(sc::ErrorCode::NotFound, "Original"));
    auto recovered = error.orElse([](const sc::Error&) {
        return sc::Result<int>::ok(100);
    });
    QVERIFY(recovered.isOk());
    QCOMPARE(recovered.unwrap(), 100);
}

void TestResult::testOptionalMethods() {
    sc::Result<int> success(42);
    QVERIFY(success.ok().has_value());
    QCOMPARE(success.ok().value(), 42);
    QVERIFY(!success.err().has_value());

    sc::Result<int> error(sc::Error(sc::ErrorCode::NotFound, "Error"));
    QVERIFY(!error.ok().has_value());
    QVERIFY(error.err().has_value());
    QCOMPARE(error.err().value().code(), sc::ErrorCode::NotFound);
}

QTEST_MAIN(TestResult)
#include "test_result.moc"