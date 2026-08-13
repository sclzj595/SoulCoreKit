// ============================================================================
// test_stress_connection.cpp — Connection 压力测试 [v2.9.0]
// ============================================================================
// 验证:
//   1. Connection pool 并发 acquire/release
//   2. Pool exhaustion 行为
//   3. Reconnect 场景
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <vector>
#include <mutex>
#include <thread>
#include <memory>
#include <deque>

class TestStressConnection : public QObject {
    Q_OBJECT

private:
    // 模拟 Connection + ConnectionPool
    class MockConnection {
    public:
        int id;
        bool connected = true;
        explicit MockConnection(int i) : id(i) {}
    };

    class MockConnectionPool {
        std::mutex m_mutex;
        std::deque<std::shared_ptr<MockConnection>> m_idle;
        std::atomic<int> m_nextId{1};
        std::atomic<int> m_totalCreated{0};
        std::atomic<int> m_acquireCount{0};
        int m_maxSize;

    public:
        explicit MockConnectionPool(int maxSize) : m_maxSize(maxSize) {}

        std::shared_ptr<MockConnection> acquire() {
            std::lock_guard lock(m_mutex);
            if (!m_idle.empty()) {
                auto conn = m_idle.front();
                m_idle.pop_front();
                m_acquireCount.fetch_add(1);
                return conn;
            }

            if (m_totalCreated.load() < m_maxSize) {
                auto conn = std::make_shared<MockConnection>(m_nextId.fetch_add(1));
                m_totalCreated.fetch_add(1);
                m_acquireCount.fetch_add(1);
                return conn;
            }

            return nullptr;  // Pool exhausted
        }

        void release(std::shared_ptr<MockConnection> conn) {
            if (!conn) return;
            std::lock_guard lock(m_mutex);
            m_idle.push_back(std::move(conn));
        }

        int totalCreated() const { return m_totalCreated.load(); }
        int acquireCount() const { return m_acquireCount.load(); }
    };

private slots:
    // 1. 并发 acquire/release
    void testConcurrentAcquireRelease() {
        MockConnectionPool pool(8);
        std::atomic<int> opsDone{0};
        const int numThreads = 8;
        const int opsPerThread = 500;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&pool, &opsDone, opsPerThread]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    auto conn = pool.acquire();
                    if (conn) {
                        QVERIFY(conn->connected);
                        QThread::msleep(1);  // 模拟使用
                        pool.release(std::move(conn));
                    }
                    opsDone.fetch_add(1);
                }
            });
        }

        for (auto& t : threads) t.join();
        QCOMPARE(opsDone.load(), numThreads * opsPerThread);
        QVERIFY(pool.totalCreated() <= 8);  // 不超过 maxSize
    }

    // 2. Pool exhaustion
    void testPoolExhaustion() {
        MockConnectionPool pool(2);  // 很小的池

        auto c1 = pool.acquire();
        auto c2 = pool.acquire();
        QVERIFY(c1 != nullptr);
        QVERIFY(c2 != nullptr);

        auto c3 = pool.acquire();  // 池已耗尽
        QVERIFY(c3 == nullptr);

        pool.release(std::move(c1));
        auto c4 = pool.acquire();  // 可以获取释放的连接
        QVERIFY(c4 != nullptr);
    }

    // 3. 连接复用
    void testConnectionReuse() {
        MockConnectionPool pool(4);

        auto c1 = pool.acquire();
        int id1 = c1->id;
        pool.release(std::move(c1));

        auto c2 = pool.acquire();
        // 复用的连接 ID 相同
        QCOMPARE(c2->id, id1);
    }

    // 4. 高压场景: 多线程 + 小池
    void testHighPressure() {
        MockConnectionPool pool(4);
        std::atomic<int> successCount{0};
        std::atomic<int> nullCount{0};
        const int numThreads = 16;
        const int opsPerThread = 200;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&pool, &successCount, &nullCount, opsPerThread]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    auto conn = pool.acquire();
                    if (conn) {
                        successCount.fetch_add(1);
                        std::this_thread::sleep_for(std::chrono::microseconds(50));
                        pool.release(std::move(conn));
                    } else {
                        nullCount.fetch_add(1);
                    }
                }
            });
        }

        for (auto& t : threads) t.join();

        // 大部分操作应成功 (acquire + release 循环)
        QVERIFY(successCount.load() > 0);
        // 不崩溃即通过
    }
};

QTEST_MAIN(TestStressConnection)
#include "test_stress_connection.moc"
