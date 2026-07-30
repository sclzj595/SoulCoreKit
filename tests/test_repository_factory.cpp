#include <QTest>
#include <QCoreApplication>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QString>

#include "soul/data/repository_factory.h"
#include "soul/orm/reflection_macros.h"
#include "soul/orm/sql_dialect.h"
#include "soul/core/result.h"

using namespace sc;

// ============================================================================
// 测试用实体
// ============================================================================

// 注意: SC_DEFINE_REFLECTION 生成 getPropertyImpl/setPropertyImpl 成员方法,
// 不依赖 Entity<T> 基类,因此可直接在普通 struct 上使用。
struct TestUser {
    QString id;
    QString name;
    int age = 0;

    SC_DEFINE_REFLECTION(TestUser,
        SC_REF_FIELD("id",   &TestUser::id)
        SC_REF_FIELD("name", &TestUser::name)
        SC_REF_FIELD("age",  &TestUser::age)
    )
};

// ============================================================================
// TestRepositoryFactory — Repository 自动代理测试
// ============================================================================
class TestRepositoryFactory : public QObject {
    Q_OBJECT

private:
    data::AutoRepository<TestUser, QString>* autoRepo() {
        return static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
    }

    QSqlDatabase m_db;
    std::shared_ptr<orm::ISqlDialect> m_dialect;
    std::unique_ptr<data::IRepository<TestUser, QString>> m_repo;

    void createTable() {
        QSqlQuery query(m_db);
        query.exec(QStringLiteral(
            "CREATE TABLE IF NOT EXISTS test_users ("
            "  id TEXT PRIMARY KEY,"
            "  name TEXT,"
            "  age INTEGER"
            ")"));
    }

    void dropTable() {
        QSqlQuery query(m_db);
        query.exec(QStringLiteral("DROP TABLE IF EXISTS test_users"));
    }

private slots:
    void initTestCase() {
        // 创建 SQLite 内存数据库
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                          QStringLiteral("test_repo_factory"));
        m_db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(m_db.open());

        // 创建方言
        m_dialect = std::shared_ptr<orm::ISqlDialect>(
            orm::ISqlDialect::create(orm::SqlDialectType::SQLite).release());

        // 创建表
        createTable();

        // 创建 Repository
        m_repo = data::RepositoryFactory<TestUser, QString>::create(
            m_dialect, m_db, "test_users");
    }

    void cleanupTestCase() {
        m_repo.reset();
        m_dialect.reset();
        m_db.close();
    }

    void cleanup() {
        QSqlQuery query(m_db);
        query.exec(QStringLiteral("DELETE FROM test_users"));
    }

    // ========================================================================
    // 基础 CRUD
    // ========================================================================

    void testSaveAndFindById() {
        TestUser user;
        user.id = "user-1";
        user.name = "Alice";
        user.age = 30;

        auto saveResult = m_repo->save(user);
        QVERIFY(saveResult.isOk());
        QCOMPARE(saveResult.unwrap().name, QString("Alice"));

        auto findResult = m_repo->findById("user-1");
        QVERIFY(findResult.isOk());
        auto found = findResult.unwrap();
        QCOMPARE(found.id, QString("user-1"));
        QCOMPARE(found.name, QString("Alice"));
        QCOMPARE(found.age, 30);
    }

    void testFindByIdNotFound() {
        auto result = m_repo->findById("nonexistent");
        QVERIFY(result.isErr());
    }

    void testFindAll() {
        TestUser u1{"u1", "Alice", 25};
        TestUser u2{"u2", "Bob", 30};
        m_repo->save(u1);
        m_repo->save(u2);

        auto result = m_repo->findAll();
        QVERIFY(result.isOk());
        auto all = result.unwrap();
        QCOMPARE(all.size(), size_t(2));
    }

    void testFindAllEmpty() {
        auto result = m_repo->findAll();
        QVERIFY(result.isOk());
        QVERIFY(result.unwrap().empty());
    }

    void testExistsById() {
        TestUser user{"u1", "Alice", 25};
        m_repo->save(user);

        auto exists = m_repo->existsById("u1");
        QVERIFY(exists.isOk());
        QVERIFY(exists.unwrap());

        auto notExists = m_repo->existsById("nonexistent");
        QVERIFY(notExists.isOk());
        QVERIFY(!notExists.unwrap());
    }

    void testRemoveById() {
        TestUser user{"u1", "Alice", 25};
        m_repo->save(user);
        QVERIFY(m_repo->existsById("u1").unwrap());

        auto removeResult = m_repo->removeById("u1");
        QVERIFY(removeResult.isOk());
        QVERIFY(!m_repo->existsById("u1").unwrap());
    }

    void testSaveOverwrite() {
        TestUser user{"u1", "Alice", 25};
        m_repo->save(user);

        TestUser updated{"u1", "AliceUpdated", 26};
        m_repo->save(updated);

        auto found = m_repo->findById("u1").unwrap();
        QCOMPARE(found.name, QString("AliceUpdated"));
        QCOMPARE(found.age, 26);
    }

    // ========================================================================
    // 扩展方法: findByField
    // ========================================================================

    void testFindByField() {
        TestUser u1{"u1", "Alice", 25};
        TestUser u2{"u2", "Bob", 30};
        m_repo->save(u1);
        m_repo->save(u2);

        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto result = autoRepo->findByField(QStringLiteral("name"), QVariant("Bob"));
        QVERIFY(result.isOk());
        auto found = result.unwrap();
        QCOMPARE(found.id, QString("u2"));
        QCOMPARE(found.name, QString("Bob"));
    }

    void testFindByFieldNotFound() {
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto result = autoRepo->findByField(QStringLiteral("name"), QVariant("Nobody"));
        QVERIFY(result.isErr());
    }

    // ========================================================================
    // 扩展方法: findAllByField
    // ========================================================================

    void testFindAllByField() {
        TestUser u1{"u1", "Alice", 25};
        TestUser u2{"u2", "Bob", 30};
        TestUser u3{"u3", "Alice", 35};
        m_repo->save(u1);
        m_repo->save(u2);
        m_repo->save(u3);

        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto result = autoRepo->findAllByField(QStringLiteral("name"), QVariant("Alice"));
        QVERIFY(result.isOk());
        auto all = result.unwrap();
        QCOMPARE(all.size(), size_t(2));
    }

    void testFindAllByFieldEmpty() {
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto result = autoRepo->findAllByField(QStringLiteral("name"), QVariant("Nobody"));
        QVERIFY(result.isOk());
        QVERIFY(result.unwrap().empty());
    }

    // ========================================================================
    // 扩展方法: update
    // ========================================================================

    void testUpdate() {
        TestUser user{"u1", "Alice", 25};
        m_repo->save(user);

        TestUser modified{"u1", "AliceUpdated", 26};
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto updateResult = autoRepo->update(modified);
        QVERIFY(updateResult.isOk());

        auto found = m_repo->findById("u1").unwrap();
        QCOMPARE(found.name, QString("AliceUpdated"));
        QCOMPARE(found.age, 26);
    }

    void testUpdateNonexistent() {
        TestUser modified{"nonexistent", "Ghost", 99};
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto result = autoRepo->update(modified);
        // UPDATE 不影响 0 行不算错误,返回成功
        QVERIFY(result.isOk());
    }

    // ========================================================================
    // 扩展方法: pageQuery
    // ========================================================================

    void testPageQuery() {
        for (int i = 1; i <= 10; ++i) {
            TestUser user{QString("u%1").arg(i), QString("User%1").arg(i), 20 + i};
            m_repo->save(user);
        }

        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());

        // 第 1 页,每页 3 条
        auto page1 = autoRepo->pageQuery(1, 3, QStringLiteral("id"));
        QVERIFY(page1.isOk());
        QCOMPARE(page1.unwrap().size(), size_t(3));

        // 第 2 页,每页 3 条
        auto page2 = autoRepo->pageQuery(2, 3, QStringLiteral("id"));
        QVERIFY(page2.isOk());
        QCOMPARE(page2.unwrap().size(), size_t(3));

        // 第 4 页,每页 3 条(只剩 1 条)
        auto page4 = autoRepo->pageQuery(4, 3, QStringLiteral("id"));
        QVERIFY(page4.isOk());
        QCOMPARE(page4.unwrap().size(), size_t(1));
    }

    void testPageQueryEmpty() {
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto result = autoRepo->pageQuery(1, 10);
        QVERIFY(result.isOk());
        QVERIFY(result.unwrap().empty());
    }

    void testPageQueryWithoutOrderBy() {
        TestUser u1{"u1", "Alice", 25};
        TestUser u2{"u2", "Bob", 30};
        m_repo->save(u1);
        m_repo->save(u2);

        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto result = autoRepo->pageQuery(1, 10);
        QVERIFY(result.isOk());
        QCOMPARE(result.unwrap().size(), size_t(2));
    }

    // ========================================================================
    // 扩展方法: count
    // ========================================================================

    void testCount() {
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        auto count0 = autoRepo->count();
        QVERIFY(count0.isOk());
        QCOMPARE(count0.unwrap(), 0);

        TestUser u1{"u1", "Alice", 25};
        TestUser u2{"u2", "Bob", 30};
        m_repo->save(u1);
        m_repo->save(u2);

        auto count2 = autoRepo->count();
        QVERIFY(count2.isOk());
        QCOMPARE(count2.unwrap(), 2);
    }

    // ========================================================================
    // 杂项
    // ========================================================================

    void testTableName() {
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(m_repo.get());
        QCOMPARE(autoRepo->tableName(), std::string("test_users"));
    }

    void testSaveBatch() {
        std::vector<TestUser> users = {
            {"u1", "Alice", 25},
            {"u2", "Bob", 30},
            {"u3", "Charlie", 35}
        };
        auto result = m_repo->saveBatch(users);
        QVERIFY(result.isOk());
        QCOMPARE(result.unwrap().size(), size_t(3));
        QCOMPARE(autoRepo()->count().unwrap(), 3);
    }

    void testRemoveBatch() {
        TestUser u1{"u1", "Alice", 25};
        TestUser u2{"u2", "Bob", 30};
        m_repo->save(u1);
        m_repo->save(u2);

        auto result = m_repo->removeBatch({"u1", "u2"});
        QVERIFY(result.isOk());
        QCOMPARE(autoRepo()->count().unwrap(), 0);
    }

    void testEntityCount() {
        TestUser u1{"u1", "Alice", 25};
        m_repo->save(u1);

        auto count = m_repo->count();
        QVERIFY(count.isOk());
        QCOMPARE(count.unwrap(), 1);
    }

};

// ============================================================================
// TestRepositoryFactoryCreate — 工厂创建测试
// ============================================================================
class TestRepositoryFactoryCreate : public QObject {
    Q_OBJECT
private:
    QSqlDatabase m_db;
    std::shared_ptr<orm::ISqlDialect> m_dialect;

private slots:
    void initTestCase() {
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"),
                                          QStringLiteral("test_repo_create"));
        m_db.setDatabaseName(QStringLiteral(":memory:"));
        QVERIFY(m_db.open());
        m_dialect = std::shared_ptr<orm::ISqlDialect>(
            orm::ISqlDialect::create(orm::SqlDialectType::SQLite).release());
    }

    void cleanupTestCase() {
        m_dialect.reset();
        m_db.close();
    }

    void testCreateWithTableName() {
        auto repo = data::RepositoryFactory<TestUser, QString>::create(
            m_dialect, m_db, "custom_users");
        QVERIFY(repo != nullptr);
        auto* autoRepo = static_cast<data::AutoRepository<TestUser, QString>*>(repo.get());
        QCOMPARE(autoRepo->tableName(), std::string("custom_users"));
    }

    void testCreateWithDefaultTableName() {
        auto repo = data::RepositoryFactory<TestUser, QString>::create(
            m_dialect, m_db);
        QVERIFY(repo != nullptr);
    }
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    int result = 0;
    { TestRepositoryFactoryCreate t; result |= QTest::qExec(&t, argc, argv); }
    { TestRepositoryFactory t; result |= QTest::qExec(&t, argc, argv); }

    return result;
}

#include "test_repository_factory.moc"