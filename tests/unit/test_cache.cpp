#include <QTest>
// v3.0.0: migrated to canonical cache
#include "soul/cache/memory_cache.h"

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
    sc::cache::MemoryCache<QString, QString>::Config config;
    config.maxEntries = 100;
    sc::cache::MemoryCache<QString, QString> cache(config);
    
    auto putResult = cache.put("key1", "value1");
    QVERIFY(putResult.isOk());
    
    auto getResult = cache.get("key1");
    QVERIFY(getResult.isOk());
    QVERIFY(getResult.unwrap().has_value());
    QCOMPARE(getResult.unwrap().value(), QString("value1"));
    
    auto missingResult = cache.get("nonexistent");
    QVERIFY(missingResult.isOk());
    QVERIFY(!missingResult.unwrap().has_value());
}

void TestMemoryCache::testContains() {
    sc::cache::MemoryCache<QString, QString>::Config config;
    config.maxEntries = 100;
    sc::cache::MemoryCache<QString, QString> cache(config);
    
    cache.put("key1", "value1");
    auto containsResult = cache.contains("key1");
    QVERIFY(containsResult.isOk());
    QVERIFY(containsResult.unwrap());
    
    auto missingResult = cache.contains("nonexistent");
    QVERIFY(missingResult.isOk());
    QVERIFY(!missingResult.unwrap());
}

void TestMemoryCache::testRemove() {
    sc::cache::MemoryCache<QString, QString>::Config config;
    config.maxEntries = 100;
    sc::cache::MemoryCache<QString, QString> cache(config);
    
    cache.put("key1", "value1");
    QVERIFY(cache.contains("key1").unwrap());
    
    auto removeResult = cache.remove("key1");
    QVERIFY(removeResult.isOk());
    QVERIFY(!cache.contains("key1").unwrap());
    
    auto removeMissingResult = cache.remove("nonexistent");
    QVERIFY(removeMissingResult.isErr());
}

void TestMemoryCache::testClear() {
    sc::cache::MemoryCache<QString, QString>::Config config;
    config.maxEntries = 100;
    sc::cache::MemoryCache<QString, QString> cache(config);
    
    cache.put("key1", "value1");
    cache.put("key2", "value2");
    QCOMPARE(cache.size(), static_cast<size_t>(2));
    
    auto clearResult = cache.clear();
    QVERIFY(clearResult.isOk());
    QCOMPARE(cache.size(), static_cast<size_t>(0));
    QVERIFY(!cache.contains("key1").unwrap());
}

void TestMemoryCache::testSize() {
    sc::cache::MemoryCache<QString, QString>::Config config;
    config.maxEntries = 100;
    sc::cache::MemoryCache<QString, QString> cache(config);
    
    QCOMPARE(cache.size(), static_cast<size_t>(0));
    
    cache.put("key1", "value1");
    QCOMPARE(cache.size(), static_cast<size_t>(1));
    
    cache.put("key2", "value2");
    QCOMPARE(cache.size(), static_cast<size_t>(2));
}

void TestMemoryCache::testLruEviction() {
    sc::cache::MemoryCache<QString, QString>::Config config;
    config.maxEntries = 3;
    sc::cache::MemoryCache<QString, QString> cache(config);
    
    cache.put("key1", "value1");
    cache.put("key2", "value2");
    cache.put("key3", "value3");
    QCOMPARE(cache.size(), static_cast<size_t>(3));
    
    // Access key1 to make it most recently used
    (void)cache.get("key1");
    cache.put("key4", "value4");
    
    QVERIFY(!cache.contains("key2").unwrap());
    QVERIFY(cache.contains("key1").unwrap());
    QVERIFY(cache.contains("key3").unwrap());
    QVERIFY(cache.contains("key4").unwrap());
    QCOMPARE(cache.size(), static_cast<size_t>(3));
}

void TestMemoryCache::testTtlExpiry() {
    sc::cache::MemoryCache<QString, QString>::Config config;
    config.maxEntries = 100;
    sc::cache::MemoryCache<QString, QString> cache(config);
    
    cache.put(QString("key1"), QString("value1"), std::chrono::seconds(1));
    
    auto containsResult = cache.contains(QString("key1"));
    QVERIFY(containsResult.isOk());
    QVERIFY(containsResult.unwrap());
    
    auto getResult = cache.get(QString("key1"));
    QVERIFY(getResult.isOk());
}

QTEST_MAIN(TestMemoryCache)
#include "test_cache.moc"
