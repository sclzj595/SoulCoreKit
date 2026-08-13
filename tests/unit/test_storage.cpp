#include <QTest>
#include "soul/storage/memory_storage.h"

class TestStorage : public QObject {
    Q_OBJECT

private slots:
    void testOpenAndClose();
    void testPutAndGet();
    void testRemove();
    void testContains();
    void testBytesOperations();
    void testClear();
    void testKeys();
};

void TestStorage::testOpenAndClose() {
    sc::MemoryStorage storage;
    auto result = storage.open("test");
    QVERIFY(result.isOk());
    QVERIFY(storage.isOpen());

    storage.close();
    QVERIFY(!storage.isOpen());
}

void TestStorage::testPutAndGet() {
    sc::MemoryStorage storage;
    auto openResult = storage.open("test");
    QVERIFY(openResult.isOk());

    auto putResult = storage.put("key", "value");
    QVERIFY(putResult.isOk());

    auto getResult = storage.get("key");
    QVERIFY(getResult.isOk());
    QCOMPARE(getResult.unwrap(), QString("value"));

    auto missingResult = storage.get("nonexistent");
    QVERIFY(missingResult.isErr());
    QCOMPARE(missingResult.unwrapErr().code(), sc::ErrorCode::NotFound);
}

void TestStorage::testRemove() {
    sc::MemoryStorage storage;
    storage.open("test");

    storage.put("key", "value");
    QVERIFY(storage.contains("key"));

    auto removeResult = storage.remove("key");
    QVERIFY(removeResult.isOk());
    QVERIFY(!storage.contains("key"));
}

void TestStorage::testContains() {
    sc::MemoryStorage storage;
    storage.open("test");

    QVERIFY(!storage.contains("key"));
    storage.put("key", "value");
    QVERIFY(storage.contains("key"));
}

void TestStorage::testBytesOperations() {
    sc::MemoryStorage storage;
    storage.open("test");

    QByteArray data = "binary data";
    auto putResult = storage.putBytes("binary_key", data);
    QVERIFY(putResult.isOk());

    auto getResult = storage.getBytes("binary_key");
    QVERIFY(getResult.isOk());
    QCOMPARE(getResult.unwrap(), data);

    auto missingResult = storage.getBytes("missing_key");
    QVERIFY(missingResult.isErr());
}

void TestStorage::testClear() {
    sc::MemoryStorage storage;
    storage.open("test");

    storage.put("key1", "value1");
    storage.put("key2", "value2");
    QCOMPARE(storage.count(), 2);

    auto clearResult = storage.clear();
    QVERIFY(clearResult.isOk());
    QCOMPARE(storage.count(), 0);
}

void TestStorage::testKeys() {
    sc::MemoryStorage storage;
    storage.open("test");

    storage.put("key1", "value1");
    storage.put("key2", "value2");
    storage.put("key3", "value3");

    auto keys = storage.keys();
    QCOMPARE(keys.size(), 3);
}

QTEST_MAIN(TestStorage)
#include "test_storage.moc"