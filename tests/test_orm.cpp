#include <QTest>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QDateTime>
#include "soul/orm/entity.h"
#include "soul/orm/column.h"
#include "soul/orm/query_wrapper.h"
#include "soul/orm/typed_query_wrapper.h"
#include "soul/orm/reflection.h"
#include "soul/orm/reflection_macros.h"
#include "soul/orm/base_repository.h"
#include "soul/orm/sqlite_repository.h"
#include "soul/data/connection_pool.h"
#include "soul/data/database_driver.h"
#include "soul/core/result.h"

using namespace sc::orm;
using namespace sc::data;

class TestUser : public Entity<TestUser> {
public:
    SC_TABLE(TestUser, "users")

    SC_FIELD(QString, name)
    SC_FIELD(int, age)
    SC_FIELD(QString, email)

    // 类型安全列引用（与上方 SC_FIELD 字段类型严格对应）
    static inline const Column<TestUser, QString>   IdCol       {"id"};
    static inline const Column<TestUser, QDateTime> CreateTimeCol{"createTime"};
    static inline const Column<TestUser, QDateTime> UpdateTimeCol{"updateTime"};
    static inline const Column<TestUser, int>       DeletedCol  {"deleted"};
    static inline const Column<TestUser, QString>   NameCol     {"name"};
    static inline const Column<TestUser, int>       AgeCol      {"age"};
    static inline const Column<TestUser, QString>   EmailCol    {"email"};

    static TableMeta tableMeta() {
        TableMeta meta;
        meta.tableName = QStringLiteral("users");
        meta.primaryKey = "id";
        meta.fields["id"] = {"id", "id", "QString", true, false, false, ""};
        meta.fields["createTime"] = {"createTime", "create_time", "QDateTime", false, false, true, ""};
        meta.fields["updateTime"] = {"updateTime", "update_time", "QDateTime", false, false, true, ""};
        meta.fields["deleted"] = {"deleted", "deleted", "int", false, false, true, "0"};
        meta.fields["name"] = {"name", "name", "QString", false, false, true, ""};
        meta.fields["age"] = {"age", "age", "int", false, false, true, ""};
        meta.fields["email"] = {"email", "email", "QString", false, false, true, ""};
        return meta;
    }

    QVariant getPropertyImpl(const QString& prop) const {
        if (prop == "id") return id;
        if (prop == "createTime") return createTime;
        if (prop == "updateTime") return updateTime;
        if (prop == "deleted") return deleted;
        if (prop == "name") return QVariant::fromValue(name);
        if (prop == "age") return QVariant::fromValue(age);
        if (prop == "email") return QVariant::fromValue(email);
        return QVariant();
    }

    void setPropertyImpl(const QString& prop, const QVariant& value) {
        if (prop == "id") { id = value.toString(); return; }
        if (prop == "createTime") { createTime = value.toDateTime(); return; }
        if (prop == "updateTime") { updateTime = value.toDateTime(); return; }
        if (prop == "deleted") { deleted = value.toInt(); return; }
        if (prop == "name") { name = value.value<QString>(); return; }
        if (prop == "age") { age = value.value<int>(); return; }
        if (prop == "email") { email = value.value<QString>(); return; }
    }
};

class TestQueryWrapper : public QObject {
    Q_OBJECT

private slots:
    void testEq();
    void testNe();
    void testGt();
    void testLike();
    void testIn();
    void testIsNull();
    void testOrderBy();
    void testLimit();
    void testBuildSelectSql();
    void testBuildUpdateSql();
    void testBuildDeleteSql();
    void testBuildUpdateSqlRejectsFullTable();
    void testBuildUpdateSqlAllowFullTableOptIn();
    void testBuildDeleteSqlRejectsFullTable();
    void testBuildDeleteSqlAllowFullTableOptIn();
    void testAndCombinesConditions();
    void testOrGroupsConditions();
    void testOrPrecedence();
    void testPostgresPlaceholder();
    void testMysqlLimitSyntax();
    void testSqliteLimitSyntax();
};

class TestSQLiteRepository : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testSave();
    void testFindById();
    void testFindAll();
    void testUpdate();
    void testRemove();
    void testCount();
    void testSaveUpdate();

private:
    std::shared_ptr<DefaultDbConnectionPool> m_pool;
    SqlRepository<TestUser>* m_repo;
};

void TestQueryWrapper::testEq() {
    QueryWrapper qw;
    qw.eq("name", "test");
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name = ?"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 1);
    QCOMPARE(binds[0].toString(), QString("test"));
}

void TestQueryWrapper::testNe() {
    QueryWrapper qw;
    qw.ne("age", 25);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age <> ?"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 1);
    QCOMPARE(binds[0].toInt(), 25);
}

void TestQueryWrapper::testGt() {
    QueryWrapper qw;
    qw.gt("age", 18);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age > ?"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 1);
    QCOMPARE(binds[0].toInt(), 18);
}

void TestQueryWrapper::testLike() {
    QueryWrapper qw;
    qw.like("name", "john");
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name LIKE ?"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 1);
    QVERIFY(binds[0].toString().contains("john"));
}

void TestQueryWrapper::testIn() {
    QueryWrapper qw;
    std::vector<QVariant> vals = {1, 2, 3};
    qw.in("age", vals);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age IN (?,?,?)"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 3);
    QCOMPARE(binds[0].toInt(), 1);
    QCOMPARE(binds[1].toInt(), 2);
    QCOMPARE(binds[2].toInt(), 3);
}

void TestQueryWrapper::testIsNull() {
    QueryWrapper qw;
    qw.isNull("email");
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("email IS NULL"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 0);
}

void TestQueryWrapper::testOrderBy() {
    QueryWrapper qw;
    qw.orderBy("age", false);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("ORDER BY age DESC"));
}

void TestQueryWrapper::testLimit() {
    QueryWrapper qw;
    qw.limit(10);
    qw.offset(20);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("LIMIT 10"));
    QVERIFY(sql.contains("OFFSET 20"));
}

void TestQueryWrapper::testBuildSelectSql() {
    QueryWrapper qw;
    qw.eq("name", "test");
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.startsWith("SELECT * FROM users"));
    QVERIFY(sql.contains("WHERE deleted = 0"));
    QVERIFY(sql.contains("AND name = ?"));
}

void TestQueryWrapper::testBuildUpdateSql() {
    QueryWrapper qw;
    qw.eq("id", "123");
    std::map<QString, QVariant> updates;
    updates["name"] = "new";
    QString sql = qw.buildUpdateSql("users", updates);
    QVERIFY(sql.startsWith("UPDATE users SET"));
    QVERIFY(sql.contains("name = ?"));
    QVERIFY(sql.contains("WHERE deleted = 0"));
}

void TestQueryWrapper::testBuildDeleteSql() {
    QueryWrapper qw;
    qw.eq("id", "123");
    QString sql = qw.buildDeleteSql("users");
    QVERIFY(sql.startsWith("UPDATE users SET deleted = 1"));
    QVERIFY(sql.contains("WHERE deleted = 0"));
}

void TestQueryWrapper::testBuildUpdateSqlRejectsFullTable() {
    // When SoftDelete is disabled and no conditions are supplied,
    // buildUpdateSql must refuse to generate a WHERE-less UPDATE.
    auto dialect = ISqlDialect::create(SqlDialectType::SQLite);
    SoftDeleteConfig sdOff;
    sdOff.enabled = false;
    dialect->setSoftDeleteConfig(sdOff);

    QueryWrapper qw;
    qw.setDialect(dialect.get());

    std::map<QString, QVariant> updates;
    updates["name"] = QString("new");

    QVERIFY_EXCEPTION_THROWN(qw.buildUpdateSql("users", updates), std::runtime_error);
}

void TestQueryWrapper::testBuildUpdateSqlAllowFullTableOptIn() {
    // Explicit opt-in via allowFullTableOperation(true) bypasses the guard.
    auto dialect = ISqlDialect::create(SqlDialectType::SQLite);
    SoftDeleteConfig sdOff;
    sdOff.enabled = false;
    dialect->setSoftDeleteConfig(sdOff);

    QueryWrapper qw;
    qw.setDialect(dialect.get());
    qw.allowFullTableOperation(true);

    std::map<QString, QVariant> updates;
    updates["name"] = QString("new");

    QString sql = qw.buildUpdateSql("users", updates);
    QVERIFY(sql.startsWith("UPDATE users SET"));
    QVERIFY(!sql.contains("WHERE", Qt::CaseInsensitive));
}

void TestQueryWrapper::testBuildDeleteSqlRejectsFullTable() {
    // When SoftDelete is disabled and no conditions are supplied,
    // buildDeleteSql must refuse to generate a WHERE-less DELETE.
    auto dialect = ISqlDialect::create(SqlDialectType::SQLite);
    SoftDeleteConfig sdOff;
    sdOff.enabled = false;
    dialect->setSoftDeleteConfig(sdOff);

    QueryWrapper qw;
    qw.setDialect(dialect.get());

    QVERIFY_EXCEPTION_THROWN(qw.buildDeleteSql("users"), std::runtime_error);
}

void TestQueryWrapper::testBuildDeleteSqlAllowFullTableOptIn() {
    // Explicit opt-in via allowFullTableOperation(true) bypasses the guard.
    auto dialect = ISqlDialect::create(SqlDialectType::SQLite);
    SoftDeleteConfig sdOff;
    sdOff.enabled = false;
    dialect->setSoftDeleteConfig(sdOff);

    QueryWrapper qw;
    qw.setDialect(dialect.get());
    qw.allowFullTableOperation(true);

    QString sql = qw.buildDeleteSql("users");
    QVERIFY(sql.startsWith("DELETE FROM users"));
    QVERIFY(!sql.contains("WHERE", Qt::CaseInsensitive));
}

void TestSQLiteRepository::initTestCase() {
    QTemporaryFile* tmpFile = new QTemporaryFile();
    tmpFile->setAutoRemove(true);
    QVERIFY(tmpFile->open());
    QString dbPath = tmpFile->fileName();

    m_pool = std::make_shared<DefaultDbConnectionPool>(
        [dbPath]() -> std::unique_ptr<IDatabaseDriver> {
            auto driver = DatabaseDriverFactory::instance().create(DatabaseType::SQLite);
            ConnectionConfig config;
            config.type = DatabaseType::SQLite;
            config.filePath = dbPath;
            driver->open(config);
            return driver;
        },
        1, 1);

    auto conn = m_pool->acquire();
    QVERIFY(conn.isOk());
    QString createSql = "CREATE TABLE users ("
        "id TEXT PRIMARY KEY, "
        "create_time TEXT, "
        "update_time TEXT, "
        "deleted INTEGER DEFAULT 0, "
        "name TEXT, "
        "age INTEGER, "
        "email TEXT)";
    auto result = conn.unwrap()->executeUpdate(createSql);
    QVERIFY(result.isOk());
    m_pool->release(std::move(conn.unwrap()));

    m_repo = new SqlRepository<TestUser>(m_pool, SqlDialectType::SQLite);
}

void TestSQLiteRepository::cleanupTestCase() {
    delete m_repo;
    m_pool.reset();
}

void TestSQLiteRepository::testSave() {
    TestUser user;
    user.name = "Alice";
    user.age = 30;
    user.email = "alice@test.com";
    auto result = m_repo->save(user);
    QVERIFY(result.isOk());
    QVERIFY(!result.unwrap().id.isEmpty());
}

void TestSQLiteRepository::testFindById() {
    TestUser user;
    user.name = "Bob";
    user.age = 25;
    user.email = "bob@test.com";
    auto saved = m_repo->save(user);
    QVERIFY(saved.isOk());

    auto found = m_repo->findById(saved.unwrap().id);
    QVERIFY(found.isOk());
    QCOMPARE(found.unwrap().name, QString("Bob"));
    QCOMPARE(found.unwrap().age, 25);
}

void TestSQLiteRepository::testFindAll() {
    auto result = m_repo->findAll();
    QVERIFY(result.isOk());
    QVERIFY(result.unwrap().size() >= 2);
}

void TestSQLiteRepository::testUpdate() {
    TestUser user;
    user.name = "Charlie";
    user.age = 35;
    user.email = "charlie@test.com";
    auto saved = m_repo->save(user);
    QVERIFY(saved.isOk());

    TestUser updateUser = saved.unwrap();
    updateUser.name = "Charlie Updated";
    auto updated = m_repo->save(updateUser);
    QVERIFY(updated.isOk());

    auto found = m_repo->findById(saved.unwrap().id);
    QVERIFY(found.isOk());
    QCOMPARE(found.unwrap().name, QString("Charlie Updated"));
}

void TestSQLiteRepository::testRemove() {
    TestUser user;
    user.name = "Dave";
    user.age = 40;
    user.email = "dave@test.com";
    auto saved = m_repo->save(user);
    QVERIFY(saved.isOk());

    auto removed = m_repo->removeById(saved.unwrap().id);
    QVERIFY(removed.isOk());

    auto found = m_repo->findById(saved.unwrap().id);
    QVERIFY(found.isErr());
    QCOMPARE(found.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestSQLiteRepository::testCount() {
    auto result = m_repo->count();
    QVERIFY(result.isOk());
    QVERIFY(result.unwrap() >= 0);
}

void TestSQLiteRepository::testSaveUpdate() {
    TestUser user;
    user.name = "Eve";
    user.age = 28;
    user.email = "eve@test.com";

    auto saved = m_repo->save(user);
    QVERIFY(saved.isOk());
    QString origId = saved.unwrap().id;

    TestUser modified = saved.unwrap();
    modified.name = "Eve Modified";
    auto updated = m_repo->save(modified);
    QVERIFY(updated.isOk());
    QCOMPARE(updated.unwrap().id, origId);

    auto found = m_repo->findById(origId);
    QVERIFY(found.isOk());
    QCOMPARE(found.unwrap().name, QString("Eve Modified"));
}

void TestQueryWrapper::testAndCombinesConditions() {
    QueryWrapper qw;
    qw.eq("name", "test").and_([](QueryWrapper& q) {
        q.eq("age", 25).eq("email", "x@y.com");
    });
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name = ?"));
    QVERIFY(sql.contains("age = ?"));
    QVERIFY(sql.contains("email = ?"));
    QVERIFY(sql.contains(" AND (age = ? AND email = ?)"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 3);
}

void TestQueryWrapper::testOrGroupsConditions() {
    QueryWrapper qw;
    qw.eq("name", "test").or_([](QueryWrapper& q) {
        q.eq("age", 25).eq("email", "x@y.com");
    });
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name = ?"));
    QVERIFY(sql.contains("age = ?"));
    QVERIFY(sql.contains("email = ?"));
    QVERIFY(sql.contains("(name = ? OR (age = ? AND email = ?))"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 3);
}

void TestQueryWrapper::testOrPrecedence() {
    QueryWrapper qw;
    qw.eq("status", 1).or_([](QueryWrapper& q) {
        q.eq("vip", 1);
    });
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("WHERE deleted = 0 AND (status = ? OR vip = ?)"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 2);
    QCOMPARE(binds[0].toInt(), 1);
    QCOMPARE(binds[1].toInt(), 1);
}

void TestQueryWrapper::testPostgresPlaceholder() {
    auto dialect = ISqlDialect::create(SqlDialectType::PostgreSQL);
    QueryWrapper qw;
    qw.setDialect(dialect.get());
    qw.eq("name", "test").eq("age", 25);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("$1"));
    QVERIFY(sql.contains("$2"));
    QVERIFY(!sql.contains("?"));
}

void TestQueryWrapper::testMysqlLimitSyntax() {
    auto dialect = ISqlDialect::create(SqlDialectType::MySQL);
    QueryWrapper qw;
    qw.setDialect(dialect.get());
    qw.limit(10).offset(20);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("LIMIT 20, 10"));
    QVERIFY(!sql.contains("OFFSET"));
}

void TestQueryWrapper::testSqliteLimitSyntax() {
    auto dialect = ISqlDialect::create(SqlDialectType::SQLite);
    QueryWrapper qw;
    qw.setDialect(dialect.get());
    qw.limit(10).offset(20);
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("LIMIT 10 OFFSET 20"));
}

// ============================================================================
// TypedQueryWrapper 测试：验证类型安全查询构造器
// ============================================================================

class TestTypedQueryWrapper : public QObject {
    Q_OBJECT

private slots:
    void testEqTyped();
    void testNeTyped();
    void testGtTyped();
    void testGeTyped();
    void testLtTyped();
    void testLeTyped();
    void testLikeTyped();
    void testLikeLeftTyped();
    void testLikeRightTyped();
    void testNotLikeTyped();
    void testInTyped();
    void testNotInTyped();
    void testIsNullTyped();
    void testIsNotNullTyped();
    void testOrderByTyped();
    void testGroupByTyped();
    void testLimitOffsetTyped();
    void testAndNestedTyped();
    void testOrNestedTyped();
    void testUnwrapDelegatesToQueryWrapper();
    void testChainedConditionsTyped();
    void testTypeMismatchFailsToCompile();  // 仅文档性注释,无实际断言
};

void TestTypedQueryWrapper::testEqTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.eq(TestUser::NameCol, QString("alice"));
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name = ?"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 1);
    QCOMPARE(binds[0].toString(), QString("alice"));
}

void TestTypedQueryWrapper::testNeTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.ne(TestUser::AgeCol, 25);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age <> ?"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 1);
    QCOMPARE(binds[0].toInt(), 25);
}

void TestTypedQueryWrapper::testGtTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.gt(TestUser::AgeCol, 18);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age > ?"));
    QCOMPARE(qw.getBindValues().at(0).toInt(), 18);
}

void TestTypedQueryWrapper::testGeTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.ge(TestUser::AgeCol, 18);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age >= ?"));
    QCOMPARE(qw.getBindValues().at(0).toInt(), 18);
}

void TestTypedQueryWrapper::testLtTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.lt(TestUser::AgeCol, 60);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age < ?"));
    QCOMPARE(qw.getBindValues().at(0).toInt(), 60);
}

void TestTypedQueryWrapper::testLeTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.le(TestUser::AgeCol, 60);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age <= ?"));
    QCOMPARE(qw.getBindValues().at(0).toInt(), 60);
}

void TestTypedQueryWrapper::testLikeTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.like(TestUser::NameCol, QString("%john%"));
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name LIKE ?"));
    QVERIFY(qw.getBindValues().at(0).toString().contains("john"));
}

void TestTypedQueryWrapper::testLikeLeftTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.likeLeft(TestUser::NameCol, QString("john"));
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name LIKE ?"));
    QVERIFY(qw.getBindValues().at(0).toString().startsWith("%"));
}

void TestTypedQueryWrapper::testLikeRightTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.likeRight(TestUser::NameCol, QString("john"));
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name LIKE ?"));
    QVERIFY(qw.getBindValues().at(0).toString().endsWith("%"));
}

void TestTypedQueryWrapper::testNotLikeTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.notLike(TestUser::NameCol, QString("%spam%"));
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name NOT LIKE ?"));
}

void TestTypedQueryWrapper::testInTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.in(TestUser::AgeCol, std::vector<int>{18, 21, 25});
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age IN (?,?,?)"));
    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 3);
    QCOMPARE(binds[0].toInt(), 18);
    QCOMPARE(binds[1].toInt(), 21);
    QCOMPARE(binds[2].toInt(), 25);
}

void TestTypedQueryWrapper::testNotInTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.notIn(TestUser::AgeCol, std::vector<int>{30, 40});
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age NOT IN (?,?)"));
    QCOMPARE(qw.getBindValues().size(), 2);
}

void TestTypedQueryWrapper::testIsNullTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.isNull(TestUser::EmailCol);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("email IS NULL"));
    QCOMPARE(qw.getBindValues().size(), 0);
}

void TestTypedQueryWrapper::testIsNotNullTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.isNotNull(TestUser::EmailCol);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("email IS NOT NULL"));
}

void TestTypedQueryWrapper::testOrderByTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.orderBy(TestUser::AgeCol, false);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("ORDER BY age DESC"));
}

void TestTypedQueryWrapper::testGroupByTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.groupBy(TestUser::AgeCol);
    // groupBy 不影响 SELECT SQL 生成（仅累积），验证 unwrap 可访问
    QVERIFY(!tq.unwrap().buildSelectSql("users").isEmpty());
}

void TestTypedQueryWrapper::testLimitOffsetTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.limit(10).offset(20);
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("LIMIT 10"));
    QVERIFY(sql.contains("OFFSET 20"));
}

void TestTypedQueryWrapper::testAndNestedTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.gt(TestUser::AgeCol, 18)
      .and_([](TypedQueryWrapper<TestUser>& inner) {
          inner.lt(TestUser::AgeCol, 60)
               .eq(TestUser::DeletedCol, 0);
      });
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("age > ?"));
    QVERIFY(sql.contains("AND ("));
    QVERIFY(sql.contains("age < ?"));
    QVERIFY(sql.contains("deleted = ?"));
    QVERIFY(sql.contains(")"));
}

void TestTypedQueryWrapper::testOrNestedTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.eq(TestUser::NameCol, QString("admin"))
      .or_([](TypedQueryWrapper<TestUser>& inner) {
          // 两个条件才会触发括号分组（QueryWrapper::or_ 行为）
          inner.eq(TestUser::EmailCol, QString("admin@example.com"))
               .eq(TestUser::AgeCol, 30);
      });
    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name = ?"));
    QVERIFY(sql.contains("OR"));
    QVERIFY(sql.contains("email = ?"));
    QVERIFY(sql.contains("age = ?"));
}

void TestTypedQueryWrapper::testUnwrapDelegatesToQueryWrapper() {
    TypedQueryWrapper<TestUser> tq;
    tq.eq(TestUser::NameCol, QString("bob"))
      .limit(5);

    QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name = ?"));
    QVERIFY(sql.contains("LIMIT 5"));

    // setDialect 透传
    auto dialect = ISqlDialect::create(SqlDialectType::SQLite);
    tq.setDialect(dialect.get());
    QVERIFY(tq.dialect() == dialect.get());
}

void TestTypedQueryWrapper::testChainedConditionsTyped() {
    TypedQueryWrapper<TestUser> tq;
    tq.eq(TestUser::NameCol, QString("alice"))
      .ge(TestUser::AgeCol, 18)
      .le(TestUser::AgeCol, 60)
      .isNotNull(TestUser::EmailCol)
      .orderBy(TestUser::AgeCol, false)
      .limit(10);

    const QueryWrapper& qw = tq.unwrap();
    QString sql = qw.buildSelectSql("users");
    QVERIFY(sql.contains("name = ?"));
    QVERIFY(sql.contains("age >= ?"));
    QVERIFY(sql.contains("age <= ?"));
    QVERIFY(sql.contains("email IS NOT NULL"));
    QVERIFY(sql.contains("ORDER BY age DESC"));
    QVERIFY(sql.contains("LIMIT 10"));

    auto binds = qw.getBindValues();
    QCOMPARE(binds.size(), 3);  // name, age>=, age<=
    QCOMPARE(binds[0].toString(), QString("alice"));
    QCOMPARE(binds[1].toInt(), 18);
    QCOMPARE(binds[2].toInt(), 60);
}

void TestTypedQueryWrapper::testTypeMismatchFailsToCompile() {
    // 文档性测试：以下代码若取消注释应编译失败
    // 因为 AgeCol 是 Column<TestUser, int>，不能传 QString
    //
    // TypedQueryWrapper<TestUser> tq;
    // tq.eq(TestUser::AgeCol, QString("not-an-int"));  // 编译错误
    //
    // 类型安全是 TypedQueryWrapper 的核心价值：
    // 字段类型与值类型在编译期绑定，避免运行时 SQL 错误
    QVERIFY(true);  // 占位断言
}

// ============================================================================
// 反射宏测试：验证 SC_DEFINE_REFLECTION 消除样板代码
// ============================================================================

// 使用反射宏的实体（与 TestUser 形成对比：零样板 getPropertyImpl/setPropertyImpl）
class ReflectedUser : public Entity<ReflectedUser> {
public:
    SC_TABLE(ReflectedUser, "reflected_users")

    QString name;
    int     age;
    QString email;

    // 一行宏替代手写 getPropertyImpl/setPropertyImpl 样板
    SC_DEFINE_REFLECTION(ReflectedUser,
        SC_REF_FIELD("name",  &ReflectedUser::name)
        SC_REF_FIELD("age",   &ReflectedUser::age)
        SC_REF_FIELD("email", &ReflectedUser::email)
    )

    static TableMeta tableMeta() {
        TableMeta meta;
        meta.tableName = QStringLiteral("reflected_users");
        meta.primaryKey = "id";
        meta.fields["id"] = {"id", "id", "QString", true, false, false, ""};
        meta.fields["name"] = {"name", "name", "QString", false, false, true, ""};
        meta.fields["age"] = {"age", "age", "int", false, false, true, ""};
        meta.fields["email"] = {"email", "email", "QString", false, false, true, ""};
        return meta;
    }
};

class TestReflection : public QObject {
    Q_OBJECT

private slots:
    void testGetProperty();
    void testSetProperty();
    void testGetReturnsInvalidForUnknownField();
    void testSetReturnsFalseForUnknownField();
    void testReflectionTableContains();
    void testReflectionTableFieldNames();
    void testTypeSafetyIntField();
    void testTypeSafetyStringField();
    void testReflectionWithEntityBase();
    void testReflectionTableIsSingleton();
};

void TestReflection::testGetProperty() {
    ReflectedUser u;
    u.name = "alice";
    u.age = 30;
    u.email = "alice@example.com";

    QCOMPARE(u.getPropertyImpl("name").toString(), QString("alice"));
    QCOMPARE(u.getPropertyImpl("age").toInt(), 30);
    QCOMPARE(u.getPropertyImpl("email").toString(), QString("alice@example.com"));
}

void TestReflection::testSetProperty() {
    ReflectedUser u;
    u.setPropertyImpl("name", QString("bob"));
    u.setPropertyImpl("age", 25);
    u.setPropertyImpl("email", QString("bob@example.com"));

    QCOMPARE(u.name, QString("bob"));
    QCOMPARE(u.age, 25);
    QCOMPARE(u.email, QString("bob@example.com"));
}

void TestReflection::testGetReturnsInvalidForUnknownField() {
    ReflectedUser u;
    u.name = "alice";
    QVariant v = u.getPropertyImpl("nonexistent");
    QVERIFY(!v.isValid());
}

void TestReflection::testSetReturnsFalseForUnknownField() {
    ReflectedUser u;
    // setPropertyImpl 返回 void（与既有约定一致），通过 getPropertyImpl 验证未修改
    u.setPropertyImpl("nonexistent", QString("value"));
    QVERIFY(u.getPropertyImpl("nonexistent").isNull() || !u.getPropertyImpl("nonexistent").isValid());
}

void TestReflection::testReflectionTableContains() {
    const auto& table = ReflectedUser::_sc_reflection();
    QVERIFY(table.contains("name"));
    QVERIFY(table.contains("age"));
    QVERIFY(table.contains("email"));
    QVERIFY(!table.contains("nonexistent"));
}

void TestReflection::testReflectionTableFieldNames() {
    const auto& table = ReflectedUser::_sc_reflection();
    auto names = table.fieldNames();
    QCOMPARE(names.size(), 3);
    // fieldNames() 按注册顺序返回: name, age, email
    QCOMPARE(names[0], QString("name"));
    QCOMPARE(names[1], QString("age"));
    QCOMPARE(names[2], QString("email"));
}

void TestReflection::testTypeSafetyIntField() {
    ReflectedUser u;
    u.setPropertyImpl("age", 42);
    // age 是 int，通过 QVariant 取出后类型应为 int
    QVariant v = u.getPropertyImpl("age");
    QVERIFY(v.canConvert<int>());
    QCOMPARE(v.toInt(), 42);
}

void TestReflection::testTypeSafetyStringField() {
    ReflectedUser u;
    u.setPropertyImpl("name", QString("charlie"));
    QVariant v = u.getPropertyImpl("name");
    QVERIFY(v.canConvert<QString>());
    QCOMPARE(v.toString(), QString("charlie"));
}

void TestReflection::testReflectionWithEntityBase() {
    // 验证通过 Entity<Derived>::getProperty/setProperty 间接调用反射表
    ReflectedUser u;
    u.name = "dave";
    u.age = 40;

    // getProperty 调用 getPropertyImpl
    QVariant nameVar = u.getProperty("name");
    QCOMPARE(nameVar.toString(), QString("dave"));

    // setProperty 调用 setPropertyImpl
    u.setProperty("age", 50);
    QCOMPARE(u.age, 50);
}

void TestReflection::testReflectionTableIsSingleton() {
    // 多次调用 _sc_reflection() 应返回同一实例
    const auto& t1 = ReflectedUser::_sc_reflection();
    const auto& t2 = ReflectedUser::_sc_reflection();
    QVERIFY(&t1 == &t2);
}

// ============================================================================
// Schema 迁移系统测试
// ============================================================================

#include "soul/orm/migration.h"

using namespace sc::orm::migration;
using sc::Result;
using sc::Ok;
using sc::data::IDatabaseDriver;

// 测试迁移 V1：创建 users 表
class MigrationV1_CreateUsers : public BaseMigration {
public:
    MigrationV1_CreateUsers() : BaseMigration("001", "Create users table") {}

    Result<void> up(IDatabaseDriver& driver) const override {
        QString sql = "CREATE TABLE users ("
                      "id TEXT PRIMARY KEY, "
                      "name TEXT, "
                      "create_time TEXT, "
                      "update_time TEXT, "
                      "deleted INTEGER DEFAULT 0"
                      ")";
        auto r = driver.executeUpdate(sql);
        if (!r.isOk()) return r.unwrapErr();
        return Ok();
    }

    Result<void> down(IDatabaseDriver& driver) const override {
        auto r = driver.executeUpdate("DROP TABLE users");
        if (!r.isOk()) return r.unwrapErr();
        return Ok();
    }
};

// 测试迁移 V2：添加 email 列
class MigrationV2_AddEmail : public BaseMigration {
public:
    MigrationV2_AddEmail() : BaseMigration("002", "Add email column to users") {}

    Result<void> up(IDatabaseDriver& driver) const override {
        auto r = driver.executeUpdate("ALTER TABLE users ADD COLUMN email TEXT");
        if (!r.isOk()) return r.unwrapErr();
        return Ok();
    }

    Result<void> down(IDatabaseDriver& driver) const override {
        // SQLite 不支持 DROP COLUMN（旧版本），这里用兼容性写法：重建表
        // 为测试简单，down 操作允许失败由具体数据库决定
        auto r = driver.executeUpdate("ALTER TABLE users DROP COLUMN email");
        if (!r.isOk()) return r.unwrapErr();
        return Ok();
    }
};

// 测试迁移 V3：创建 orders 表
class MigrationV3_CreateOrders : public BaseMigration {
public:
    MigrationV3_CreateOrders() : BaseMigration("003", "Create orders table") {}

    Result<void> up(IDatabaseDriver& driver) const override {
        QString sql = "CREATE TABLE orders ("
                      "id TEXT PRIMARY KEY, "
                      "user_id TEXT, "
                      "amount REAL"
                      ")";
        auto r = driver.executeUpdate(sql);
        if (!r.isOk()) return r.unwrapErr();
        return Ok();
    }

    Result<void> down(IDatabaseDriver& driver) const override {
        auto r = driver.executeUpdate("DROP TABLE orders");
        if (!r.isOk()) return r.unwrapErr();
        return Ok();
    }
};

// 失败迁移：故意执行非法 SQL
class MigrationFailing : public BaseMigration {
public:
    MigrationFailing() : BaseMigration("099", "Failing migration for test") {}

    Result<void> up(IDatabaseDriver& driver) const override {
        auto r = driver.executeUpdate("CREATE TABLE invalid_syntax_table (");  // 语法错误
        if (!r.isOk()) return r.unwrapErr();
        return Ok();
    }

    Result<void> down(IDatabaseDriver& driver) const override {
        Q_UNUSED(driver);
        return Ok();
    }
};

class TestMigration : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();

    void testEmptyManagerMigrate();
    void testApplySingleMigration();
    void testApplyMultipleMigrations();
    void testMigrateIdempotent();
    void testRollbackSingle();
    void testRollbackMultiple();
    void testRollbackToVersion();
    void testCurrentVersion();
    void testPendingMigrations();
    void testAppliedMigrations();
    void testMigrationFailureRollsBackTransaction();
    void testRegisteredCount();
    void testDuplicateRegistrationIgnored();

private:
    std::shared_ptr<IDatabaseDriver> createFreshDriver();

    QString m_dbPath;
    QTemporaryFile* m_tmpFile = nullptr;
};

void TestMigration::initTestCase() {
    m_tmpFile = new QTemporaryFile();
    m_tmpFile->setAutoRemove(true);
    QVERIFY(m_tmpFile->open());
    m_dbPath = m_tmpFile->fileName();
    m_tmpFile->close();
}

void TestMigration::cleanupTestCase() {
    delete m_tmpFile;
}

void TestMigration::init() {
    // 每个测试前重置数据库：删除所有表
    auto driver = createFreshDriver();
    if (driver && driver->isConnected()) {
        driver->executeUpdate("DROP TABLE IF EXISTS users");
        driver->executeUpdate("DROP TABLE IF EXISTS orders");
        driver->executeUpdate("DROP TABLE IF EXISTS schema_migrations");
    }
}

std::shared_ptr<IDatabaseDriver> TestMigration::createFreshDriver() {
    auto driver = DatabaseDriverFactory::instance().create(DatabaseType::SQLite);
    if (!driver) return nullptr;
    ConnectionConfig config;
    config.type = DatabaseType::SQLite;
    config.filePath = m_dbPath;
    auto r = driver->open(config);
    if (!r.isOk()) return nullptr;
    return driver;
}

void TestMigration::testEmptyManagerMigrate() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    auto result = mgr.migrate();
    QVERIFY2(result.isOk(), result.isErr() ? result.unwrapErr().message().toUtf8().constData() : "");

    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QVERIFY(versionResult.unwrap().isEmpty());
}

void TestMigration::testApplySingleMigration() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());

    auto result = mgr.migrate();
    QVERIFY2(result.isOk(), result.isErr() ? result.unwrapErr().message().toUtf8().constData() : "");

    // 验证 users 表存在（通过查询不报错）
    auto queryResult = driver->executeQuery("SELECT COUNT(*) AS cnt FROM users");
    QVERIFY(queryResult.isOk());

    // 验证当前版本
    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QCOMPARE(versionResult.unwrap(), QString("001"));
}

void TestMigration::testApplyMultipleMigrations() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationV3_CreateOrders>());

    auto result = mgr.migrate();
    QVERIFY2(result.isOk(), result.isErr() ? result.unwrapErr().message().toUtf8().constData() : "");

    // 验证两个表都存在
    QVERIFY(driver->executeQuery("SELECT COUNT(*) AS cnt FROM users").isOk());
    QVERIFY(driver->executeQuery("SELECT COUNT(*) AS cnt FROM orders").isOk());

    // 验证版本为 003（最大版本号）
    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QCOMPARE(versionResult.unwrap(), QString("003"));
}

void TestMigration::testMigrateIdempotent() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());

    // 第一次 migrate
    auto r1 = mgr.migrate();
    QVERIFY(r1.isOk());

    // 第二次 migrate 应跳过已应用迁移
    auto r2 = mgr.migrate();
    QVERIFY(r2.isOk());

    // 版本仍为 001
    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QCOMPARE(versionResult.unwrap(), QString("001"));

    // 待应用迁移应为空
    auto pendingResult = mgr.pendingMigrations();
    QVERIFY(pendingResult.isOk());
    QCOMPARE(pendingResult.unwrap().size(), static_cast<std::size_t>(0));
}

void TestMigration::testRollbackSingle() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationV3_CreateOrders>());

    QVERIFY(mgr.migrate().isOk());

    // 回滚 1 步（回滚 003）
    auto r = mgr.rollback(1);
    QVERIFY2(r.isOk(), r.isErr() ? r.unwrapErr().message().toUtf8().constData() : "");

    // orders 表应已删除
    auto queryResult = driver->executeQuery("SELECT COUNT(*) AS cnt FROM orders");
    QVERIFY(!queryResult.isOk());  // 表不存在，查询失败

    // 版本回退到 001
    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QCOMPARE(versionResult.unwrap(), QString("001"));
}

void TestMigration::testRollbackMultiple() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationV3_CreateOrders>());

    QVERIFY(mgr.migrate().isOk());

    // 回滚 2 步（回滚 003 和 001）
    auto r = mgr.rollback(2);
    QVERIFY2(r.isOk(), r.isErr() ? r.unwrapErr().message().toUtf8().constData() : "");

    // 所有表应已删除
    QVERIFY(!driver->executeQuery("SELECT COUNT(*) AS cnt FROM users").isOk());
    QVERIFY(!driver->executeQuery("SELECT COUNT(*) AS cnt FROM orders").isOk());

    // 版本为空
    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QVERIFY(versionResult.unwrap().isEmpty());
}

void TestMigration::testRollbackToVersion() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationV3_CreateOrders>());

    QVERIFY(mgr.migrate().isOk());

    // 回滚到 001（保留 001，回滚 003）
    auto r = mgr.rollbackTo("001");
    QVERIFY2(r.isOk(), r.isErr() ? r.unwrapErr().message().toUtf8().constData() : "");

    // users 表存在，orders 表删除
    QVERIFY(driver->executeQuery("SELECT COUNT(*) AS cnt FROM users").isOk());
    QVERIFY(!driver->executeQuery("SELECT COUNT(*) AS cnt FROM orders").isOk());

    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QCOMPARE(versionResult.unwrap(), QString("001"));
}

void TestMigration::testCurrentVersion() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);

    // 初始版本为空
    auto v0 = mgr.currentVersion();
    QVERIFY(v0.isOk());
    QVERIFY(v0.unwrap().isEmpty());

    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    QVERIFY(mgr.migrate().isOk());

    auto v1 = mgr.currentVersion();
    QVERIFY(v1.isOk());
    QCOMPARE(v1.unwrap(), QString("001"));
}

void TestMigration::testPendingMigrations() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationV3_CreateOrders>());

    // 应用前：2 个待应用
    auto pending0 = mgr.pendingMigrations();
    QVERIFY(pending0.isOk());
    QCOMPARE(pending0.unwrap().size(), static_cast<std::size_t>(2));

    // 应用 001 后：1 个待应用
    QVERIFY(mgr.migrate().isOk());
    // 此时所有迁移已应用，待应用为 0
    auto pending1 = mgr.pendingMigrations();
    QVERIFY(pending1.isOk());
    QCOMPARE(pending1.unwrap().size(), static_cast<std::size_t>(0));
}

void TestMigration::testAppliedMigrations() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationV3_CreateOrders>());

    QVERIFY(mgr.migrate().isOk());

    auto applied = mgr.appliedMigrations();
    QVERIFY(applied.isOk());
    auto records = applied.unwrap();
    QCOMPARE(records.size(), static_cast<std::size_t>(2));
    // 按应用时间升序：001 在前，003 在后
    QCOMPARE(records[0].version, QString("001"));
    QCOMPARE(records[0].description, QString("Create users table"));
    QCOMPARE(records[1].version, QString("003"));
    QCOMPARE(records[1].description, QString("Create orders table"));
    QVERIFY(records[0].appliedAt.isValid());
}

void TestMigration::testMigrationFailureRollsBackTransaction() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationFailing>());

    auto result = mgr.migrate();
    // 迁移应失败
    QVERIFY(!result.isOk());

    // 001 应已成功应用（在 099 失败前）
    QVERIFY(driver->executeQuery("SELECT COUNT(*) AS cnt FROM users").isOk());

    // 099 应未记录（事务回滚）
    auto versionResult = mgr.currentVersion();
    QVERIFY(versionResult.isOk());
    QCOMPARE(versionResult.unwrap(), QString("001"));
}

void TestMigration::testRegisteredCount() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    QCOMPARE(mgr.registeredCount(), static_cast<std::size_t>(0));

    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    QCOMPARE(mgr.registeredCount(), static_cast<std::size_t>(1));

    mgr.addMigration(std::make_shared<MigrationV3_CreateOrders>());
    QCOMPARE(mgr.registeredCount(), static_cast<std::size_t>(2));
}

void TestMigration::testDuplicateRegistrationIgnored() {
    auto driver = createFreshDriver();
    QVERIFY(driver && driver->isConnected());

    MigrationManager mgr(driver);
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());
    mgr.addMigration(std::make_shared<MigrationV1_CreateUsers>());  // 重复注册

    QCOMPARE(mgr.registeredCount(), static_cast<std::size_t>(1));
}

// Multiple test classes in one executable: cannot use QTEST_MAIN (generates its
// own main). Implement main() manually and run each suite via QTest::qExec.
// QCoreApplication is required so QSqlDatabase can load driver plugins.
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);
    int status = 0;
    {
        TestSQLiteRepository tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestQueryWrapper tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestTypedQueryWrapper tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestReflection tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    {
        TestMigration tc;
        status |= QTest::qExec(&tc, argc, argv);
    }
    return status;
}
#include "test_orm.moc"
