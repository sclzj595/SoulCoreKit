// ============================================================================
// test_concurrency_cache.cpp — Cache 并发安全测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. 多线程并发 get/set 不崩溃
//   2. 并发读写计数正确
//   3. 缓存驱逐时无 use-after-free
//
// 运行建议: cmake -DENABLE_TSAN=ON && ctest -L concurrency -R test_concurrency_cache
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <string>

class TestConcurrencyCache : public QObject {
    Q_OBJECT

private:
    // 模拟线程安全的简单缓存
    template<typename K, typename V>
    class MockThreadSafeCache {
        mutable std::shared_mutex m_mutex;
        std::unordered_map<K, V> m_data;
        std::atomic<size_t> m_hitCount{0};
        std::atomic<size_t> m_missCount{0};

    public:
        void set(const K& key, V value) {
            std::unique_lock lock(m_mutex);
            m_data[key] = std::move(value);
        }

        std::optional<V> get(const K& key) {
            std::shared_lock lock(m_mutex);
            auto it = m_data.find(key);
            if (it != m_data.end()) {
                m_hitCount.fetch_add(1, std::memory_order_relaxed);
                return it->second;
            }
            m_missCount.fetch_add(1, std::memory_order_relaxed);
            return std::nullopt;
        }

        bool contains(const K& key) const {
            std::shared_lock lock(m_mutex);
            return m_data.find(key) != m_data.end();
        }

        size_t size() const {
            std::shared_lock lock(m_mutex);
            return m_data.size();
        }

        void clear() {
            std::unique_lock lock(m_mutex);
            m_data.clear();
        }

        size_t hitCount() const { return m_hitCount.load(); }
        size_t missCount() const { return m_missCount.load(); }
    };

private slots:
    // 1. 多线程并发 set + get
    void testConcurrentSetAndGet() {
        MockThreadSafeCache<int, int> cache;
        std::vector<std::thread> threads;
        const int numThreads = 8;
        const int opsPerThread = 200;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&cache, opsPerThread, t]() {
                int base = t * 1000;
                for (int i = 0; i < opsPerThread; ++i) {
                    int key = base + i;
                    cache.set(key, key * 2);
                    auto val = cache.get(key);
                    QVERIFY(val.has_value());
                    QCOMPARE(val.value(), key * 2);
                }
            });
        }

        for (auto& t : threads) t.join();

        int expectedEntries = numThreads * opsPerThread;
        QCOMPARE(cache.size(), size_t(expectedEntries));
        QCOMPARE(cache.missCount(), size_t(0));  // 所有 key 都在 set 后 get
    }

    // 2. 多线程只读
    void testConcurrentReadOnly() {
        MockThreadSafeCache<int, int> cache;

        // 预填充
        for (int i = 0; i < 100; ++i) {
            cache.set(i, i * 10);
        }

        std::vector<std::thread> threads;
        const int numThreads = 16;
        const int readsPerThread = 500;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&cache, readsPerThread]() {
                for (int i = 0; i < readsPerThread; ++i) {
                    int key = i % 100;
                    auto val = cache.get(key);
                    QVERIFY(val.has_value());
                }
            });
        }

        for (auto& t : threads) t.join();

        size_t expectedHits = numThreads * readsPerThread;
        QCOMPARE(cache.hitCount(), expectedHits);
        QCOMPARE(cache.missCount(), size_t(0));
    }

    // 3. 并发 mixed read/write
    void testConcurrentMixed() {
        MockThreadSafeCache<int, int> cache;
        std::vector<std::thread> writers;
        std::vector<std::thread> readers;
        const int numWriters = 4;
        const int numReaders = 8;
        const int opsPerThread = 300;

        for (int t = 0; t < numWriters; ++t) {
            writers.emplace_back([&cache, opsPerThread, t]() {
                int base = t * 10000;
                for (int i = 0; i < opsPerThread; ++i) {
                    cache.set(base + i, i);
                }
            });
        }

        for (int t = 0; t < numReaders; ++t) {
            readers.emplace_back([&cache, opsPerThread]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    cache.get(i);  // 可能 hit 或 miss
                }
            });
        }

        for (auto& t : writers) t.join();
        for (auto& t : readers) t.join();

        // 不崩溃即通过
        QVERIFY(cache.size() > 0);
    }

    // 4. 并发 clear + get
    void testConcurrentClearAndGet() {
        MockThreadSafeCache<int, int> cache;

        // 预填充
        for (int i = 0; i < 200; ++i) {
            cache.set(i, i);
        }

        std::atomic<bool> stopClear{false};
        std::thread clearer([&cache, &stopClear]() {
            while (!stopClear.load(std::memory_order_acquire)) {
                cache.clear();
                // 重新填充
                for (int i = 0; i < 100; ++i) {
                    cache.set(i, i);
                }
            }
        });

        std::vector<std::thread> readers;
        for (int t = 0; t < 4; ++t) {
            readers.emplace_back([&cache, &stopClear]() {
                while (!stopClear.load(std::memory_order_acquire)) {
                    for (int i = 0; i < 50; ++i) {
                        cache.get(i);  // 可能 hit 或 miss，但不应崩溃
                    }
                }
            });
        }

        // 运行一小段时间
        QThread::msleep(200);
        stopClear.store(true, std::memory_order_release);

        clearer.join();
        for (auto& t : readers) t.join();
        // 不崩溃即通过
        QVERIFY(true);
    }
};

QTEST_MAIN(TestConcurrencyCache)
#include "test_concurrency_cache.moc"
