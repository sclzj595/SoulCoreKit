#include <QTest>
#include "soul/storage/cache.h"

class TestMemoryCache : public QObject {
    Q_OBJECT

private slots:
    void testPutAndGet();
    void testContains();
    void testRemove();
    void testClear();
    void testSize();
    void testLruEviction();
    void testTtlExpiry();
};

void TestMemoryCache::testPutAndGet() {
    sc::MemoryCache<QString, QString> cache(100);
    
    auto putResult = cache.put("key1", "value1");
    QVERIFY(putResult.isOk());
    
    auto getResult = cache.get("key1");
    QVERIFY(getResult.isOk());
    QCOMPARE(getResult.unwrap(), QString("value1"));
    
    auto missingResult = cache.get("nonexistent");
    QVERIFY(missingResult.isErr());
}

void TestMemoryCache::testContains() {
    sc::MemoryCache<QString, QString> cache(100);
    
    cache.put("key1", "value1");
    QVERIFY(cache.contains("key1"));
    QVERIFY(!cache.contains("nonexistent"));
}

void TestMemoryCache::testRemove() {
    sc::MemoryCache<QString, QString> cache(100);
    
    cache.put("key1", "value1");
    QVERIFY(cache.contains("key1"));
    
    auto removeResult = cache.remove("key1");
    QVERIFY(removeResult.isOk());
    QVERIFY(!cache.contains("key1"));
    
    auto removeMissingResult = cache.remove("nonexistent");
    QVERIFY(removeMissingResult.isErr());
}

void TestMemoryCache::testClear() {
    sc::MemoryCache<QString, QString> cache(100);
    
    cache.put("key1", "value1");
    cache.put("key2", "value2");
    QCOMPARE(cache.size(), static_cast<size_t>(2));
    
    cache.clear();
    QCOMPARE(cache.size(), static_cast<size_t>(0));
    QVERIFY(!cache.contains("key1"));
}

void TestMemoryCache::testSize() {
    sc::MemoryCache<QString, QString> cache(100);
    
    QCOMPARE(cache.size(), static_cast<size_t>(0));
    
    cache.put("key1", "value1");
    QCOMPARE(cache.size(), static_cast<size_t>(1));
    
    cache.put("key2", "value2");
    QCOMPARE(cache.size(), static_cast<size_t>(2));
    
    QCOMPARE(cache.maxSize(), static_cast<size_t>(100));
}

void TestMemoryCache::testLruEviction() {
    sc::MemoryCache<QString, QString> cache(3);
    
    cache.put("key1", "value1");
    cache.put("key2", "value2");
    cache.put("key3", "value3");
    QCOMPARE(cache.size(), static_cast<size_t>(3));
    
    cache.get("key1");
    cache.put("key4", "value4");
    
    QVERIFY(!cache.contains("key2"));
    QVERIFY(cache.contains("key1"));
    QVERIFY(cache.contains("key3"));
    QVERIFY(cache.contains("key4"));
    QCOMPARE(cache.size(), static_cast<size_t>(3));
}

void TestMemoryCache::testTtlExpiry() {
    sc::MemoryCache<QString, QString> cache(100);
    
    cache.put(QString("key1"), QString("value1"), std::chrono::seconds(1));
    
    QVERIFY(cache.contains(QString("key1")));
    
    auto getResult = cache.get(QString("key1"));
    QVERIFY(getResult.isOk());
}

QTEST_MAIN(TestMemoryCache)
#include "test_cache.moc"