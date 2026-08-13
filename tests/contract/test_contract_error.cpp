// ============================================================================
// test_contract_error.cpp — Error/Result 增强模型契约测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. ErrorCategory 便捷判断方法
//   2. Error::withContext() 链式上下文
//   3. Error::context() 元数据存取
//   4. Error 不可变性 (withContext 返回新对象)
//   5. Error cause 链
// ============================================================================

#include <QtTest>
#include "soul/core/error.h"
#include "soul/core/result.h"

using namespace sc;

class TestContractError : public QObject {
    Q_OBJECT

private slots:
    // ========================================================================
    // ErrorCategory 便捷判断
    // ========================================================================

    void testCategoryHelpers() {
        Error netErr(ErrorCode::Timeout, "Connection timeout");
        QVERIFY(netErr.isNetworkError());
        QVERIFY(!netErr.isDatabaseError());
        QVERIFY(!netErr.isResourceError());
        QCOMPARE(netErr.category(), ErrorCategory::Network);

        Error dbErr(ErrorCode::QueryFailed, "SELECT failed");
        QVERIFY(dbErr.isDatabaseError());
        QVERIFY(!dbErr.isNetworkError());
        QCOMPARE(dbErr.category(), ErrorCategory::Database);

        Error resErr(ErrorCode::NotFound, "User not found");
        QVERIFY(resErr.isResourceError());
        QVERIFY(!resErr.isFileError());
        QCOMPARE(resErr.category(), ErrorCategory::Resource);

        Error fileErr(ErrorCode::FileNotFound, "config.json missing");
        QVERIFY(fileErr.isFileError());
        QCOMPARE(fileErr.category(), ErrorCategory::FileIO);

        Error parseErr(ErrorCode::ParseError, "JSON parse error");
        QVERIFY(parseErr.isParseError());
        QCOMPARE(parseErr.category(), ErrorCategory::Parse);

        Error intErr(ErrorCode::InternalError, "Something went wrong");
        QVERIFY(intErr.isInternalError());
        QCOMPARE(intErr.category(), ErrorCategory::Internal);
    }

    void testIsCategoryGeneric() {
        Error err(ErrorCode::Timeout, "timeout");
        QVERIFY(err.isCategory(ErrorCategory::Network));
        QVERIFY(!err.isCategory(ErrorCategory::Database));
        QVERIFY(!err.isCategory(ErrorCategory::None));
    }

    // ========================================================================
    // Error::withContext() 链式上下文
    // ========================================================================

    void testWithContext() {
        Error err(ErrorCode::NetworkError, "Connection lost");

        auto enriched = err
            .withContext("request_id", QString("req-12345"))
            .withContext("user_id", 42)
            .withContext("retry_count", 3);

        QCOMPARE(enriched.contextValue("request_id").toString(), QString("req-12345"));
        QCOMPARE(enriched.contextValue("user_id").toInt(), 42);
        QCOMPARE(enriched.contextValue("retry_count").toInt(), 3);

        // 原始 Error 不受影响 (不可变性)
        QVERIFY(err.context().isEmpty());
    }

    void testContextNotAffectEquality() {
        Error err(ErrorCode::Timeout, "timeout");
        auto enriched = err.withContext("key", "value");

        // code 和 message 不变
        QCOMPARE(err.code(), enriched.code());
        QCOMPARE(err.message(), enriched.message());
    }

    void testContextMissingKey() {
        Error err(ErrorCode::Unknown, "unknown");
        auto val = err.contextValue("nonexistent");
        QVERIFY(!val.isValid());
    }

    // ========================================================================
    // Error cause 链
    // ========================================================================

    void testCauseChain() {
        auto root = std::make_shared<Error>(ErrorCode::FileNotFound, "config.json not found");
        Error mid(ErrorCode::FileReadError, "Failed to read config", root);
        Error top(ErrorCode::InternalError, "Application startup failed",
                  std::make_shared<Error>(mid));

        QCOMPARE(top.code(), ErrorCode::InternalError);
        QVERIFY(top.cause() != nullptr);
        QCOMPARE(top.cause()->code(), ErrorCode::FileReadError);
        QVERIFY(top.cause()->cause() != nullptr);
        QCOMPARE(top.cause()->cause()->code(), ErrorCode::FileNotFound);
    }

    void testToStringWithCause() {
        auto cause = std::make_shared<Error>(ErrorCode::Timeout, "Socket timeout");
        Error err(ErrorCode::NetworkError, "Request failed", cause);

        // v3.0.0: 修正断言 — Error::toString() 使用数字错误码 (项目约定),
        // 而非枚举名 (参照 CsError::toString() 的数字约定)。
        QString str = err.toString();
        QVERIFY(str.contains("200"));               // NetworkError = 200
        QVERIFY(str.contains("201"));               // Timeout = 201 (cause)
        QVERIFY(str.contains("Request failed"));    // 主错误消息
        QVERIFY(str.contains("Socket timeout"));    // cause 消息
        QVERIFY(str.contains("->"));                // cause 链分隔符
    }

    // ========================================================================
    // Result<T> with ErrorCategory
    // ========================================================================

    void testResultWithCategoryError() {
        auto r = Result<int>::err(
            Error(ErrorCode::DatabaseError, "Connection pool exhausted")
        );

        QVERIFY(r.isErr());
        auto& err = r.unwrapErr();
        QVERIFY(err.isDatabaseError());
        QCOMPARE(err.category(), ErrorCategory::Database);
    }

    // ========================================================================
    // ErrorCode → ErrorCategory 边界情况
    // ========================================================================

    void testCategoryBoundaries() {
        // 精确边界值
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(99)), ErrorCategory::None);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(100)), ErrorCategory::Resource);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(199)), ErrorCategory::Resource);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(200)), ErrorCategory::Network);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(299)), ErrorCategory::Network);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(300)), ErrorCategory::Parse);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(400)), ErrorCategory::Database);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(500)), ErrorCategory::FileIO);
        QCOMPARE(categoryFromCode(static_cast<ErrorCode>(600)), ErrorCategory::Internal);
    }

    // ========================================================================
    // Error::toStdString()
    // ========================================================================

    void testToStdString() {
        Error err(ErrorCode::Ok, "Success");
        std::string s = err.toStdString();
        QVERIFY(!s.empty());
        QVERIFY(s.find("Success") != std::string::npos);
    }
};

QTEST_MAIN(TestContractError)
#include "test_contract_error.moc"
