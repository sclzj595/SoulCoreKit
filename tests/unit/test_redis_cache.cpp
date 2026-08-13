// ============================================================================
// test_redis_cache.cpp — RedisCache 接口测试 [v2.9.2]
// ============================================================================

#include <QtTest>
#include "soul/cache/redis_cache.h"

using namespace sc::cache;

class TestRedisCache : public QObject {
    Q_OBJECT

private slots:
    // 1. 构造不崩溃
    void testConstruction() {
        RedisCache cache("tcp://127.0.0.1:6379");
        auto s = cache.stats();
        QVERIFY(!s.name.empty());
    }

    // 2. get 在未连接时返回 Err (非 nullopt)
    void testGetReturnsError() {
        RedisCache cache("tcp://127.0.0.1:6379");
        auto result = cache.get("key1");
        QVERIFY(result.isErr());
        // 失败模型: Redis 不可用 → Err, 不是 Ok+nullopt
    }

    // 3. put 在未连接时返回 Err
    void testPutReturnsError() {
        RedisCache cache("tcp://127.0.0.1:6379");
        auto result = cache.put("key1", "value1");
        QVERIFY(result.isErr());
    }

    // 4. remove 在未连接时返回 Err
    void testRemoveReturnsError() {
        RedisCache cache("tcp://127.0.0.1:6379");
        QVERIFY(cache.remove("key1").isErr());
    }

    // 5. contains 在未连接时返回 Err
    void testContainsReturnsError() {
        RedisCache cache("tcp://127.0.0.1:6379");
        auto result = cache.contains("key1");
        QVERIFY(result.isErr());
    }

    // 6. clear 在未连接时返回 Err
    void testClearReturnsError() {
        RedisCache cache("tcp://127.0.0.1:6379");
        QVERIFY(cache.clear().isErr());
    }

    // 7. getMany / putMany 错误传播
    void testBatchOperations() {
        RedisCache cache("tcp://127.0.0.1:6379");

        auto getResult = cache.getMany({"a", "b"});
        QVERIFY(getResult.isErr());

        auto putResult = cache.putMany({{"a", "1"}, {"b", "2"}});
        QVERIFY(putResult.isErr());
    }

    // 8. stats 包含错误计数
    void testStatsErrorCount() {
        RedisCache cache("tcp://127.0.0.1:6379");

        cache.get("k1");
        cache.get("k2");
        cache.put("k3", "v3");

        auto s = cache.stats();
        QVERIFY(s.errorCount >= 1);
    }

    // 9. key prefix
    void testKeyPrefix() {
        RedisCache cache("tcp://127.0.0.1:6379");
        cache.setKeyPrefix("myapp:");
        // 验证不崩溃 (实际 prefix 在 makeKey 中使用)
        cache.get("user:42");  // 应使用 "myapp:user:42"
        QVERIFY(true);
    }

    // 10. default TTL
    void testDefaultTtl() {
        RedisCache cache("tcp://127.0.0.1:6379");
        cache.setDefaultTtl(std::chrono::minutes(5));
        // 验证不崩溃
        cache.put("k", "v");
        QVERIFY(true);
    }
};

QTEST_MAIN(TestRedisCache)
#include "test_redis_cache.moc"
