// ============================================================================
// test_concurrency_event.cpp — EventBus 并发安全测试 [v2.6.0]
// ============================================================================
// 验证:
//   1. 多线程并发 publish/subscribe 不崩溃
//   2. 订阅者计数正确性
//   3. 事件分发线程安全
//
// 运行建议: cmake -DENABLE_TSAN=ON && ctest -L concurrency -R test_concurrency_event
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <vector>
#include <mutex>
#include <functional>
#include <memory>

class TestConcurrencyEvent : public QObject {
    Q_OBJECT

private:
    // 模拟线程安全的事件总线
    template<typename Event>
    class MockEventBus {
        std::mutex m_mutex;
        std::vector<std::function<void(const Event&)>> m_subscribers;
        std::atomic<size_t> m_dispatchCount{0};

    public:
        void subscribe(std::function<void(const Event&)> handler) {
            std::lock_guard lock(m_mutex);
            m_subscribers.push_back(std::move(handler));
        }

        void publish(const Event& event) {
            std::vector<std::function<void(const Event&)>> snapshot;
            {
                std::lock_guard lock(m_mutex);
                snapshot = m_subscribers;
            }
            // 锁外分发，避免 Rule 3 违规
            for (auto& handler : snapshot) {
                handler(event);
            }
            m_dispatchCount.fetch_add(1, std::memory_order_relaxed);
        }

        size_t dispatchCount() const {
            return m_dispatchCount.load(std::memory_order_relaxed);
        }
    };

    struct TestEvent {
        int value;
        QString message;
    };

private slots:
    // 1. 多线程并发 publish
    void testConcurrentPublish() {
        MockEventBus<TestEvent> bus;
        std::atomic<int> receivedCount{0};

        // 注册订阅者
        bus.subscribe([&receivedCount](const TestEvent&) {
            receivedCount.fetch_add(1, std::memory_order_relaxed);
        });

        std::vector<std::thread> threads;
        const int numThreads = 8;
        const int eventsPerThread = 100;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&bus, eventsPerThread, t]() {
                for (int i = 0; i < eventsPerThread; ++i) {
                    bus.publish(TestEvent{i, QString("thread %1").arg(t)});
                }
            });
        }

        for (auto& t : threads) t.join();

        int expected = numThreads * eventsPerThread;
        QCOMPARE(receivedCount.load(), expected);
        QCOMPARE(bus.dispatchCount(), size_t(expected));
    }

    // 2. 并发 subscribe + publish 同时进行
    void testConcurrentSubscribeAndPublish() {
        MockEventBus<TestEvent> bus;
        std::atomic<int> totalReceived{0};

        std::vector<std::thread> publishers;
        std::vector<std::thread> subscribers;

        const int numPublishers = 4;
        const int numSubscribers = 4;
        const int opsPerThread = 200;

        // 先注册一个初始订阅者
        bus.subscribe([&totalReceived](const TestEvent&) {
            totalReceived.fetch_add(1, std::memory_order_relaxed);
        });

        for (int t = 0; t < numPublishers; ++t) {
            publishers.emplace_back([&bus, opsPerThread]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    bus.publish(TestEvent{i, "pub"});
                }
            });
        }

        for (int t = 0; t < numSubscribers; ++t) {
            subscribers.emplace_back([&bus, &totalReceived, opsPerThread]() {
                for (int i = 0; i < opsPerThread; ++i) {
                    bus.subscribe([&totalReceived](const TestEvent&) {
                        totalReceived.fetch_add(1, std::memory_order_relaxed);
                    });
                }
            });
        }

        for (auto& t : publishers) t.join();
        for (auto& t : subscribers) t.join();

        // 所有 publish 和 subscribe 完成不崩溃即通过
        QVERIFY(bus.dispatchCount() > 0);
        QVERIFY(totalReceived.load() > 0);
    }

    // 3. 多订阅者并发接收
    void testMultipleSubscribers() {
        MockEventBus<TestEvent> bus;
        std::atomic<int> subscriber1Count{0};
        std::atomic<int> subscriber2Count{0};
        std::atomic<int> subscriber3Count{0};

        bus.subscribe([&subscriber1Count](const TestEvent&) {
            subscriber1Count.fetch_add(1, std::memory_order_relaxed);
        });
        bus.subscribe([&subscriber2Count](const TestEvent&) {
            subscriber2Count.fetch_add(1, std::memory_order_relaxed);
        });
        bus.subscribe([&subscriber3Count](const TestEvent&) {
            subscriber3Count.fetch_add(1, std::memory_order_relaxed);
        });

        std::vector<std::thread> threads;
        const int numThreads = 4;
        const int eventsPerThread = 50;

        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&bus, eventsPerThread]() {
                for (int i = 0; i < eventsPerThread; ++i) {
                    bus.publish(TestEvent{i, "multi"});
                }
            });
        }

        for (auto& t : threads) t.join();

        int expected = numThreads * eventsPerThread;
        // 每个事件触发所有 3 个订阅者
        QCOMPARE(subscriber1Count.load(), expected);
        QCOMPARE(subscriber2Count.load(), expected);
        QCOMPARE(subscriber3Count.load(), expected);
    }

    // 4. 无订阅者 publish 不崩溃
    void testPublishWithNoSubscribers() {
        MockEventBus<TestEvent> bus;

        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&bus]() {
                for (int i = 0; i < 100; ++i) {
                    bus.publish(TestEvent{i, "no_sub"});
                }
            });
        }

        for (auto& t : threads) t.join();
        QCOMPARE(bus.dispatchCount(), size_t(400));
    }
};

QTEST_MAIN(TestConcurrencyEvent)
#include "test_concurrency_event.moc"
