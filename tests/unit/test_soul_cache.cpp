#include <QTest>
#include <QTemporaryDir>
#include <QCoreApplication>
#include "soul/cache/icache.h"
#include "soul/cache/memory_cache.h"
#include "soul/cache/disk_cache.h"
#include "soul/cache/multi_level_cache.h"

#include <chrono>
#include <thread>
#include <memory>

using namespace sc::cache;

// ---------------------------------------------------------------------------
// MemoryCache (L1)
// ---------------------------------------------------------------------------
class TestSoulMemoryCache : public QObject {
    Q_OBJECT

private slots:
    void testPutAndGet();
    void testGetMissingReturnsNullopt();
    void testRemove();
    void testContains();
    void testClear();
    void testSize();
    void testTtlExpiry();
    void testLruEvictionByEntries();
    void testLruEvictionByBytes();
    void testGetMany();
    void testPutMany();
    void testStatsHitMiss();
};

void TestSoulMemoryCache::testPutAndGet() {
    MemoryCache<int, std::string> cache;
    QVERIFY(cache.put(1, "value1").isOk());

    auto r = cache.get(1);
    QVERIFY(r.isOk());
    QVERIFY(r.unwrap().has_value());
    QCOMPARE(r.unwrap().value(), std::string("value1"));
}

void TestSoulMemoryCache::testGetMissingReturnsNullopt() {
    MemoryCache<int, std::string> cache;
    auto r = cache.get(999);
    QVERIFY(r.isOk());
    QVERIFY(!r.unwrap().has_value());
}

void TestSoulMemoryCache::testRemove() {
    MemoryCache<int, std::string> cache;
    cache.put(1, "value1");

    QVERIFY(cache.remove(1).isOk());
    QVERIFY(!cache.contains(1).unwrap());

    QVERIFY(cache.remove(999).isErr());
}

void TestSoulMemoryCache::testContains() {
    MemoryCache<int, std::string> cache;
    cache.put(1, "value1");

    QVERIFY(cache.contains(1).unwrap());
    QVERIFY(!cache.contains(999).unwrap());
}

void TestSoulMemoryCache::testClear() {
    MemoryCache<int, std::string> cache;
    cache.put(1, "a");
    cache.put(2, "b");
    QCOMPARE(cache.size(), static_cast<std::size_t>(2));

    QVERIFY(cache.clear().isOk());
    QCOMPARE(cache.size(), static_cast<std::size_t>(0));
}

void TestSoulMemoryCache::testSize() {
    MemoryCache<int, std::string> cache;
    QCOMPARE(cache.size(), static_cast<std::size_t>(0));
    cache.put(1, "a");
    QCOMPARE(cache.size(), static_cast<std::size_t>(1));
    cache.put(2, "b");
    QCOMPARE(cache.size(), static_cast<std::size_t>(2));
    cache.put(1, "updated");
    QCOMPARE(cache.size(), static_cast<std::size_t>(2));
}

void TestSoulMemoryCache::testTtlExpiry() {
    MemoryCache<int, std::string>::Config cfg;
    cfg.defaultTtl = std::chrono::milliseconds(50);
    MemoryCache<int, std::string> cache(cfg);

    cache.put(1, "short");
    QVERIFY(cache.contains(1).unwrap());

    std::this_thread::sleep_for(std::chrono::milliseconds(80));

    QVERIFY(!cache.contains(1).unwrap());
    auto r = cache.get(1);
    QVERIFY(r.isOk());
    QVERIFY(!r.unwrap().has_value());
}

void TestSoulMemoryCache::testLruEvictionByEntries() {
    MemoryCache<int, std::string>::Config cfg;
    cfg.maxEntries = 3;
    MemoryCache<int, std::string> cache(cfg);

    cache.put(1, "a");
    cache.put(2, "b");
    cache.put(3, "c");
    QCOMPARE(cache.size(), static_cast<std::size_t>(3));

    // Access key 1 to make it recently used
    (void)cache.get(1);

    // Insert key 4: should evict key 2 (LRU)
    cache.put(4, "d");
    QCOMPARE(cache.size(), static_cast<std::size_t>(3));
    QVERIFY(cache.contains(1).unwrap());
    QVERIFY(!cache.contains(2).unwrap());
    QVERIFY(cache.contains(3).unwrap());
    QVERIFY(cache.contains(4).unwrap());
}

void TestSoulMemoryCache::testLruEvictionByBytes() {
    MemoryCache<int, std::string>::Config cfg;
    cfg.maxEntries = 100;
    cfg.maxBytes = 100;
    MemoryCache<int, std::string> cache(cfg);

    cache.put(1, std::string(20, 'x'));
    cache.put(2, std::string(20, 'y'));
    cache.put(3, std::string(20, 'z'));

    QVERIFY(cache.stats().evictionCount > 0);
}

void TestSoulMemoryCache::testGetMany() {
    MemoryCache<int, std::string> cache;
    cache.put(1, "a");
    cache.put(2, "b");
    cache.put(3, "c");

    auto r = cache.getMany({1, 2, 999});
    QVERIFY(r.isOk());
    auto& hits = r.unwrap();
    QCOMPARE(hits.size(), static_cast<std::size_t>(2));
    QCOMPARE(hits[1], std::string("a"));
    QCOMPARE(hits[2], std::string("b"));
}

void TestSoulMemoryCache::testPutMany() {
    MemoryCache<int, std::string> cache;
    std::unordered_map<int, std::string> entries = {
        {1, "a"}, {2, "b"}, {3, "c"}};
    QVERIFY(cache.putMany(entries).isOk());
    QCOMPARE(cache.size(), static_cast<std::size_t>(3));

    auto r = cache.get(2);
    QCOMPARE(r.unwrap().value(), std::string("b"));
}

void TestSoulMemoryCache::testStatsHitMiss() {
    MemoryCache<int, std::string> cache;
    cache.put(1, "a");

    (void)cache.get(1);    // hit
    (void)cache.get(1);    // hit
    (void)cache.get(999);  // miss

    auto s = cache.stats();
    QCOMPARE(s.hitCount, static_cast<std::size_t>(2));
    QCOMPARE(s.missCount, static_cast<std::size_t>(1));
    QVERIFY(s.hitRate() > 0.0);
}

// ---------------------------------------------------------------------------
// DiskCache (L2)
// ---------------------------------------------------------------------------
class TestSoulDiskCache : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void testPutAndGet();
    void testGetMissing();
    void testRemove();
    void testContains();
    void testClear();
    void testTtlExpiry();
    void testGetMany();
    void testPutMany();
    void testStats();

private:
    QString m_cacheDir;
};

void TestSoulDiskCache::initTestCase() {
    static QTemporaryDir tmp;
    QVERIFY(tmp.isValid());
    m_cacheDir = tmp.path() + QStringLiteral("/soul_cache_test");
}

void TestSoulDiskCache::testPutAndGet() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p1");
    DiskCache cache(cfg);

    QVERIFY(cache.put("key1", "value1").isOk());
    auto r = cache.get("key1");
    QVERIFY(r.isOk());
    QVERIFY(r.unwrap().has_value());
    QCOMPARE(r.unwrap().value(), std::string("value1"));
}

void TestSoulDiskCache::testGetMissing() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p2");
    DiskCache cache(cfg);

    auto r = cache.get("nonexistent");
    QVERIFY(r.isOk());
    QVERIFY(!r.unwrap().has_value());
}

void TestSoulDiskCache::testRemove() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p3");
    DiskCache cache(cfg);

    cache.put("key1", "value1");
    QVERIFY(cache.remove("key1").isOk());
    QVERIFY(!cache.contains("key1").unwrap());

    QVERIFY(cache.remove("nonexistent").isErr());
}

void TestSoulDiskCache::testContains() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p4");
    DiskCache cache(cfg);

    cache.put("key1", "value1");
    QVERIFY(cache.contains("key1").unwrap());
    QVERIFY(!cache.contains("nonexistent").unwrap());
}

void TestSoulDiskCache::testClear() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p5");
    DiskCache cache(cfg);

    cache.put("k1", "v1");
    cache.put("k2", "v2");
    QVERIFY(cache.clear().isOk());

    QVERIFY(!cache.contains("k1").unwrap());
    QVERIFY(!cache.contains("k2").unwrap());
}

void TestSoulDiskCache::testTtlExpiry() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p6");
    DiskCache cache(cfg);

    cache.put("ephemeral", "data", std::chrono::milliseconds(50));
    QVERIFY(cache.contains("ephemeral").unwrap());

    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    QVERIFY(!cache.contains("ephemeral").unwrap());

    auto r = cache.get("ephemeral");
    QVERIFY(r.isOk());
    QVERIFY(!r.unwrap().has_value());
}

void TestSoulDiskCache::testGetMany() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p7");
    DiskCache cache(cfg);

    cache.put("k1", "v1");
    cache.put("k2", "v2");

    auto r = cache.getMany({"k1", "k2", "missing"});
    QVERIFY(r.isOk());
    auto& hits = r.unwrap();
    QCOMPARE(hits.size(), static_cast<std::size_t>(2));
    QCOMPARE(hits["k1"], std::string("v1"));
    QCOMPARE(hits["k2"], std::string("v2"));
}

void TestSoulDiskCache::testPutMany() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p8");
    DiskCache cache(cfg);

    std::unordered_map<std::string, std::string> entries = {
        {"a", "1"}, {"b", "2"}};
    QVERIFY(cache.putMany(entries).isOk());

    auto r = cache.get("a");
    QCOMPARE(r.unwrap().value(), std::string("1"));
}

void TestSoulDiskCache::testStats() {
    DiskCache::Config cfg;
    cfg.cacheDir = m_cacheDir + QStringLiteral("/p9");
    DiskCache cache(cfg);

    cache.put("k1", "v1");
    (void)cache.get("k1");    // hit
    (void)cache.get("missing");  // miss

    auto s = cache.stats();
    QCOMPARE(s.hitCount, static_cast<std::size_t>(1));
    QCOMPARE(s.missCount, static_cast<std::size_t>(1));
}

// ---------------------------------------------------------------------------
// MultiLevelCache
// ---------------------------------------------------------------------------
class TestSoulMultiLevelCache : public QObject {
    Q_OBJECT

private slots:
    void testL1HitDoesNotQueryL2();
    void testL1MissL2HitBackfillsL1();
    void testAllMissReturnsNullopt();
    void testPutBroadcastsToAllLevels();
    void testRemoveFromAllLevels();
    void testStatsMerged();
};

void TestSoulMultiLevelCache::testL1HitDoesNotQueryL2() {
    auto l1 = std::make_shared<MemoryCache<int, std::string>>();
    auto l2 = std::make_shared<MemoryCache<int, std::string>>();

    l1->put(1, "from-l1");

    MultiLevelCache<int, std::string> cache({l1, l2});
    auto r = cache.get(1);
    QVERIFY(r.isOk());
    QCOMPARE(r.unwrap().value(), std::string("from-l1"));

    QVERIFY(!l2->contains(1).unwrap());
}

void TestSoulMultiLevelCache::testL1MissL2HitBackfillsL1() {
    auto l1 = std::make_shared<MemoryCache<int, std::string>>();
    auto l2 = std::make_shared<MemoryCache<int, std::string>>();

    l2->put(1, "from-l2");

    MultiLevelCache<int, std::string> cache({l1, l2});
    auto r = cache.get(1);
    QVERIFY(r.isOk());
    QCOMPARE(r.unwrap().value(), std::string("from-l2"));

    QVERIFY(l1->contains(1).unwrap());
}

void TestSoulMultiLevelCache::testAllMissReturnsNullopt() {
    auto l1 = std::make_shared<MemoryCache<int, std::string>>();
    auto l2 = std::make_shared<MemoryCache<int, std::string>>();

    MultiLevelCache<int, std::string> cache({l1, l2});
    auto r = cache.get(999);
    QVERIFY(r.isOk());
    QVERIFY(!r.unwrap().has_value());
}

void TestSoulMultiLevelCache::testPutBroadcastsToAllLevels() {
    auto l1 = std::make_shared<MemoryCache<int, std::string>>();
    auto l2 = std::make_shared<MemoryCache<int, std::string>>();

    MultiLevelCache<int, std::string> cache({l1, l2});
    cache.put(1, "broadcasted");

    QVERIFY(l1->contains(1).unwrap());
    QVERIFY(l2->contains(1).unwrap());
}

void TestSoulMultiLevelCache::testRemoveFromAllLevels() {
    auto l1 = std::make_shared<MemoryCache<int, std::string>>();
    auto l2 = std::make_shared<MemoryCache<int, std::string>>();

    l1->put(1, "a");
    l2->put(1, "b");

    MultiLevelCache<int, std::string> cache({l1, l2});
    QVERIFY(cache.remove(1).isOk());

    QVERIFY(!l1->contains(1).unwrap());
    QVERIFY(!l2->contains(1).unwrap());
}

void TestSoulMultiLevelCache::testStatsMerged() {
    auto l1 = std::make_shared<MemoryCache<int, std::string>>();
    auto l2 = std::make_shared<MemoryCache<int, std::string>>();

    l1->put(1, "a");
    (void)l1->get(1);   // L1 hit
    l2->put(2, "b");
    (void)l2->get(2);   // L2 hit
    (void)l2->get(999); // L2 miss

    MultiLevelCache<int, std::string> cache({l1, l2});
    auto s = cache.stats();
    QCOMPARE(s.hitCount, static_cast<std::size_t>(2));
    QCOMPARE(s.missCount, static_cast<std::size_t>(1));
}

// ---------------------------------------------------------------------------
// Entry point: run all test classes
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    int result = 0;

    {
        TestSoulMemoryCache t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestSoulDiskCache t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestSoulMultiLevelCache t;
        result |= QTest::qExec(&t, argc, argv);
    }

    return result;
}

#include "test_soul_cache.moc"
