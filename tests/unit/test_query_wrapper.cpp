#include <QTest>
#include <QString>
#include <QVariant>
#include <QStringList>
#include <vector>

#include "soul/orm/query_wrapper.h"

using namespace sc;
using sc::orm::QueryWrapper;
using sc::orm::SqlKeyword;

// ============================================================================
// TestQueryWrapper — 查询构造器测试
// ============================================================================
class TestQueryWrapper : public QObject {
    Q_OBJECT

private slots:
    void testSelectAll();
    void testEq();
    void testMultipleConditions();
    void testAnd();
    void testOr();
    void testLike();
    void testIn();
    void testIsNull();
    void testOrderBy();
    void testLimit();
    void testGroupBy();
    void testComplexQuery();
    void testOrGroupAllConditions();
};

void TestQueryWrapper::testSelectAll() {
    QueryWrapper q;
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("SELECT *"));
    QVERIFY(sql.contains("FROM users"));
}

void TestQueryWrapper::testEq() {
    QueryWrapper q;
    q.eq("name", "Alice");
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("WHERE"));
    QVERIFY(sql.contains("name"));
}

void TestQueryWrapper::testMultipleConditions() {
    QueryWrapper q;
    q.eq("status", "active").gt("age", 18);
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("status"));
    QVERIFY(sql.contains("age"));
}

void TestQueryWrapper::testAnd() {
    QueryWrapper q;
    q.eq("status", "active");
    q.and_([&](QueryWrapper& sub) {
        sub.eq("city", "NYC");
        sub.eq("role", "admin");
    });
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("AND"));
}

void TestQueryWrapper::testOr() {
    QueryWrapper q;
    q.eq("role", "admin");
    q.or_([&](QueryWrapper& sub) {
        sub.eq("city", "NYC");
        sub.eq("city", "LA");
    });
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("OR"));
}

void TestQueryWrapper::testLike() {
    QueryWrapper q;
    q.like("name", "%Ali%");
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("LIKE"));
}

void TestQueryWrapper::testIn() {
    std::vector<QVariant> vals{QVariant("NYC"), QVariant("LA"), QVariant("SF")};
    QueryWrapper q;
    q.in("city", vals);
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("IN"));
    // v3.0.0: IN 值走参数化占位符 (防 SQL 注入), 实际值在 bind values 中,
    // 而非内联到 SQL 字符串。验证绑定值包含 "NYC"。
    auto bindValues = q.getBindValues();
    bool foundNyc = false;
    for (const auto& v : bindValues) {
        if (v.toString() == "NYC") { foundNyc = true; break; }
    }
    QVERIFY(foundNyc);
}

void TestQueryWrapper::testIsNull() {
    QueryWrapper q;
    q.isNull("deleted_at");
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("IS NULL"));
}

void TestQueryWrapper::testOrderBy() {
    QueryWrapper q;
    q.orderBy("name", true);
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("ORDER BY"));
    QVERIFY(sql.contains("name"));
}

void TestQueryWrapper::testLimit() {
    QueryWrapper q;
    q.limit(10);
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("LIMIT"));
    QVERIFY(sql.contains("10"));
}

void TestQueryWrapper::testGroupBy() {
    QueryWrapper q;
    q.groupBy("city");
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("GROUP BY"));
}

void TestQueryWrapper::testComplexQuery() {
    QueryWrapper q;
    q.eq("status", "active")
     .gt("age", 18)
     .orderBy("name", true)
     .limit(20);

    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("status"));
    QVERIFY(sql.contains("age"));
    QVERIFY(sql.contains("ORDER BY"));
    QVERIFY(sql.contains("LIMIT"));
}

void TestQueryWrapper::testOrGroupAllConditions() {
    // [审计] or_() 应把组内所有条件设为 OR, 而非仅第一个。
    QueryWrapper q;
    q.or_([&](QueryWrapper& sub) {
        sub.eq("city", "NYC");
        sub.eq("city", "LA");
        sub.eq("city", "SF");
    });
    QString sql = q.buildSelectSql("users");
    QVERIFY(sql.contains("OR"));

    // 验证生成的 SQL 中有 3 个 OR 连接(括号内), 而非 1 个 OR + 2 个 AND
    int orCount = sql.count("OR");
    QVERIFY(orCount >= 2);
}

QTEST_MAIN(TestQueryWrapper)
#include "test_query_wrapper.moc"
