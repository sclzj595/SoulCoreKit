// ============================================================================
// test_message_bus_concurrency.cpp — MessageBus 并发测试 [v2.9.1]
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <memory>
#include <vector>
#include "soul/event/inmemory_message_bus.h"

using namespace sc;

class TestMessageBusConcurrency : public QObject {
    Q_OBJECT

private slots:
    // 1. 并发 publish + subscribe
    void testConcurrentPubSub() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};

        // v3.0.0: 保存订阅句柄 (RAII)
        auto sub = bus->subscribe("test", [&received](const auto&) {
            received.fetch_add(1);
        });

        const int numThreads = 8;
        const int msgsPerThread = 500;

        std::vector<std::thread> threads;
        for (int t = 0; t < numThreads; ++t) {
            threads.emplace_back([&bus, msgsPerThread]() {
                for (int i = 0; i < msgsPerThread; ++i) {
                    bus->publish("test", Message::create("test"));
                }
            });
        }

        for (auto& t : threads) t.join();

        QCOMPARE(received.load(), numThreads * msgsPerThread);
    }

    // 2. 并发 subscribe 与 publish 同时
    void testConcurrentSubscribeWhilePublish() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};

        std::vector<std::thread> subscribers;
        std::vector<std::thread> publishers;

        for (int t = 0; t < 4; ++t) {
            publishers.emplace_back([&bus]() {
                for (int i = 0; i < 200; ++i) {
                    bus->publish("mixed", Message::create("mixed"));
                }
            });
        }

        for (int t = 0; t < 4; ++t) {
            subscribers.emplace_back([&bus, &received]() {
                for (int i = 0; i < 100; ++i) {
                    bus->subscribe("mixed", [&received](const auto&) {
                        received.fetch_add(1);
                    });
                }
            });
        }

        for (auto& t : publishers) t.join();
        for (auto& t : subscribers) t.join();

        QVERIFY(received.load() > 0);
    }

    // 3. 并发 unsubscribe 与 publish
    void testConcurrentUnsubscribeWhilePublish() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};
        std::atomic<bool> stopPublish{false};

        auto sub = bus->subscribe("test", [&received](const auto&) {
            received.fetch_add(1);
        });

        // 发布线程
        std::thread publisher([&bus, &stopPublish]() {
            while (!stopPublish.load(std::memory_order_acquire)) {
                bus->publish("test", Message::create("test"));
            }
        });

        // 频繁 unsubscribe + subscribe
        for (int i = 0; i < 50; ++i) {
            bus->unsubscribe(sub);
            sub = bus->subscribe("test", [&received](const auto&) {
                received.fetch_add(1);
            });
        }

        stopPublish.store(true, std::memory_order_release);
        publisher.join();

        QVERIFY(received.load() > 0);
    }

    // 4. 并发 unsubscribeAll 与 publish
    void testConcurrentUnsubscribeAll() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<bool> stop{false};

        for (int i = 0; i < 5; ++i) {
            bus->subscribe("ch", [](const auto&) {});
        }

        std::thread publisher([&bus, &stop]() {
            while (!stop.load(std::memory_order_acquire)) {
                bus->publish("ch", Message::create("ch"));
            }
        });

        for (int i = 0; i < 20; ++i) {
            bus->unsubscribeAll("ch");
            bus->subscribe("ch", [](const auto&) {});
        }

        stop.store(true, std::memory_order_release);
        publisher.join();
        QVERIFY(true);  // 不崩溃即通过
    }

    // 5. shutdown 期间 publish
    void testPublishDuringShutdown() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};

        bus->subscribe("test", [&received](const auto&) { received++; });

        std::atomic<bool> stop{false};
        std::thread publisher([&bus, &stop]() {
            while (!stop.load(std::memory_order_acquire)) {
                bus->publish("test", Message::create("test"));
            }
        });

        QThread::msleep(50);
        bus->shutdown();
        stop.store(true, std::memory_order_release);
        publisher.join();

        QVERIFY(bus->isShutdown());
    }

    // 6. Consumer 异常不影响其他 Consumer
    void testConsumerException() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> normal{0};

        // v3.0.0: 保存订阅句柄 (RAII)
        auto s1 = bus->subscribe("test", [](const auto&) {
            throw std::runtime_error("Consumer error");
        });
        auto s2 = bus->subscribe("test", [&normal](const auto&) { normal++; });

        bus->publish("test", Message::create("test"));
        QCOMPARE(normal.load(), 1);  // 正常 Consumer 仍然收到
    }
};

QTEST_MAIN(TestMessageBusConcurrency)
#include "test_message_bus_concurrency.moc"
