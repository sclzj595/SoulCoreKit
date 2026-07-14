#include <QTest>
#include "soul/core/result.h"

class TestResult : public QObject {
    Q_OBJECT

private slots:
    void testOkResult();
    void testErrorResult();
    void testResultVoid();
    void testResultChaining();
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

QTEST_MAIN(TestResult)
#include "test_result.moc"
