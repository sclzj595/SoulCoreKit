#include <QTest>
#include <QCoreApplication>
#include "soul/data/repository.h"
#include "soul/data/memory_repository.h"
#include "soul/data/database_driver.h"
#include "soul/data/connection_pool.h"
#include "soul/core/result.h"

struct TestEntity {
    QString id;
    QString name;
    int value;
};

class TestMemoryRepository : public QObject {
    Q_OBJECT

private slots:
    void testFindById();
    void testFindByIdNotFound();
    void testFindAll();
    void testSaveNew();
    void testSaveUpdate();
    void testRemoveById();
    void testRemoveByIdNotFound();
    void testExistsById();
    void testSaveBatch();
    void testRemoveBatch();
    void testCount();
};

void TestMemoryRepository::testFindById() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    TestEntity entity{"1", "test", 100};
    repo.save(entity);
    
    auto result = repo.findById("1");
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().id, QString("1"));
    QCOMPARE(result.unwrap().name, QString("test"));
    QCOMPARE(result.unwrap().value, 100);
}

void TestMemoryRepository::testFindByIdNotFound() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    auto result = repo.findById("nonexistent");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestMemoryRepository::testFindAll() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    repo.save({"1", "a", 1});
    repo.save({"2", "b", 2});
    repo.save({"3", "c", 3});
    
    auto result = repo.findAll();
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().size(), 3);
}

void TestMemoryRepository::testSaveNew() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    TestEntity entity{"1", "new", 42};
    auto result = repo.save(entity);
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().id, QString("1"));
}

void TestMemoryRepository::testSaveUpdate() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    repo.save({"1", "original", 10});
    repo.save({"1", "updated", 20});
    
    auto result = repo.findById("1");
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().name, QString("updated"));
    QCOMPARE(result.unwrap().value, 20);
}

void TestMemoryRepository::testRemoveById() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    repo.save({"1", "to_remove", 1});
    
    auto removeResult = repo.removeById("1");
    QVERIFY(removeResult.isOk());
    
    auto findResult = repo.findById("1");
    QVERIFY(findResult.isErr());
}

void TestMemoryRepository::testRemoveByIdNotFound() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    auto result = repo.removeById("nonexistent");
    QVERIFY(result.isErr());
    QCOMPARE(result.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestMemoryRepository::testExistsById() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    repo.save({"1", "exists", 1});
    
    auto result = repo.existsById("1");
    QVERIFY(result.isOk());
    QVERIFY(result.unwrap());
    
    result = repo.existsById("nonexistent");
    QVERIFY(result.isOk());
    QVERIFY(!result.unwrap());
}

void TestMemoryRepository::testSaveBatch() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    std::vector<TestEntity> entities = {
        {"1", "a", 1},
        {"2", "b", 2},
        {"3", "c", 3}
    };
    
    auto result = repo.saveBatch(entities);
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap().size(), 3);
}

void TestMemoryRepository::testRemoveBatch() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    repo.save({"1", "a", 1});
    repo.save({"2", "b", 2});
    
    auto removeResult = repo.removeBatch({"1", "2"});
    QVERIFY(removeResult.isOk());
    
    auto findAllResult = repo.findAll();
    QVERIFY(findAllResult.isOk());
    QVERIFY(findAllResult.unwrap().empty());
}

void TestMemoryRepository::testCount() {
    sc::data::MemoryRepository<TestEntity, QString> repo(
        [](const TestEntity& e) { return e.id; }
    );
    
    repo.save({"1", "a", 1});
    repo.save({"2", "b", 2});
    repo.save({"3", "c", 3});
    
    auto result = repo.count();
    QVERIFY(result.isOk());
    QCOMPARE(result.unwrap(), 3);
}

class TestDatabaseDriver : public QObject {
    Q_OBJECT

private slots:
    void testCreateSqliteDriver();
    void testCreateMySqlDriver();
    void testCreatePostgreSqlDriver();
};

void TestDatabaseDriver::testCreateSqliteDriver() {
    auto driver = sc::data::DatabaseDriverFactory::instance().create(sc::data::DatabaseType::SQLite);
    QVERIFY(driver != nullptr);
    QCOMPARE(driver->getType(), sc::data::DatabaseType::SQLite);
}

void TestDatabaseDriver::testCreateMySqlDriver() {
    auto driver = sc::data::DatabaseDriverFactory::instance().create(sc::data::DatabaseType::MySQL);
    QVERIFY(driver != nullptr);
    QCOMPARE(driver->getType(), sc::data::DatabaseType::MySQL);
}

void TestDatabaseDriver::testCreatePostgreSqlDriver() {
    auto driver = sc::data::DatabaseDriverFactory::instance().create(sc::data::DatabaseType::PostgreSQL);
    QVERIFY(driver != nullptr);
    QCOMPARE(driver->getType(), sc::data::DatabaseType::PostgreSQL);
}

class TestDbConnectionPool : public QObject {
    Q_OBJECT

private slots:
    void testPoolInitialization();
    void testAcquireRelease();
};

void TestDbConnectionPool::testPoolInitialization() {
    sc::data::ConnectionConfig config;
    config.type = sc::data::DatabaseType::SQLite;
    config.filePath = ":memory:";
    
    sc::data::DefaultDbConnectionPool pool(config, 2, 5);
    QCOMPARE(pool.getPoolSize(), 0);
    QCOMPARE(pool.getActiveConnections(), 0);
}

void TestDbConnectionPool::testAcquireRelease() {
    sc::data::ConnectionConfig config;
    config.type = sc::data::DatabaseType::SQLite;
    config.filePath = ":memory:";
    
    sc::data::DefaultDbConnectionPool pool(config, 2, 5);
    auto conn = pool.acquire();
    QVERIFY(conn.isOk());
    QCOMPARE(pool.getActiveConnections(), 1);
    
    pool.release(std::move(conn.unwrap()));
    QCOMPARE(pool.getActiveConnections(), 0);
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    int result = 0;

    {
        TestMemoryRepository t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestDatabaseDriver t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestDbConnectionPool t;
        result |= QTest::qExec(&t, argc, argv);
    }

    return result;
}

#include "test_data.moc"