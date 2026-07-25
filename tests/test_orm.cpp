#include <QTest>
#include <QCoreApplication>
#include <QTemporaryFile>
#include <QDateTime>
#include "soul/orm/entity.h"
#include "soul/orm/query_wrapper.h"
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

QTEST_MAIN(TestSQLiteRepository)
#include "test_orm.moc"
