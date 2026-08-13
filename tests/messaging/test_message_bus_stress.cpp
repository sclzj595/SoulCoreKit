// ============================================================================
// test_message_bus_stress.cpp — MessageBus 压力测试 [v2.9.1]
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>
#include "soul/event/inmemory_message_bus.h"

using namespace sc;

class TestMessageBusStress : public QObject {
    Q_OBJECT

private slots:
    // 1. 100 publishers + 10 subscribers + 10000 messages
    void testHighThroughput() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int64_t> delivered{0};
        std::atomic<int64_t> published{0};

        // v3.0.0: 保存订阅句柄 (RAII)
        std::vector<MessageSubscriptionPtr> subs;
        subs.reserve(10);
        for (int s = 0; s < 10; ++s) {
            subs.push_back(bus->subscribe("stress", [&delivered](const auto&) {
                delivered.fetch_add(1);
            }));
        }

        const int numPublishers = 8;
        const int msgsPerPublisher = 1250;  // 总共 10000 条

        std::vector<std::thread> threads;
        for (int t = 0; t < numPublishers; ++t) {
            threads.emplace_back([&bus, &published, msgsPerPublisher]() {
                for (int i = 0; i < msgsPerPublisher; ++i) {
                    bus->publish("stress", Message::create("stress"));
                    published.fetch_add(1);
                }
            });
        }

        for (auto& t : threads) t.join();

        int64_t expected = int64_t(numPublishers) * msgsPerPublisher * 10;
        QCOMPARE(published.load(), int64_t(numPublishers) * msgsPerPublisher);
        QCOMPARE(delivered.load(), expected);
    }

    // 2. 长时间运行
    void testLongRunning() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int64_t> delivered{0};
        std::atomic<bool> stop{false};

        // v3.0.0: 保存订阅句柄 (RAII)
        auto sub = bus->subscribe("long", [&delivered](const auto&) {
            delivered.fetch_add(1);
        });

        std::thread publisher([&bus, &stop]() {
            while (!stop.load(std::memory_order_acquire)) {
                bus->publish("long", Message::create("long"));
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        });

        QThread::msleep(1000);  // 运行 1 秒
        stop.store(true, std::memory_order_release);
        publisher.join();

        QVERIFY(delivered.load() > 0);
    }

    // 3. 多频道并发
    void testMultiChannel() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        const int numChannels = 20;
        std::vector<std::atomic<int>> counters(numChannels);

        // v3.0.0: 保存订阅句柄 (RAII)
        std::vector<MessageSubscriptionPtr> subs;
        subs.reserve(numChannels);
        for (int c = 0; c < numChannels; ++c) {
            QString channel = QString("ch.%1").arg(c);
            subs.push_back(bus->subscribe(channel, [&counters, c](const auto&) {
                counters[c].fetch_add(1);
            }));
        }

        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([&bus, numChannels]() {
                for (int i = 0; i < 500; ++i) {
                    for (int c = 0; c < numChannels; ++c) {
                        bus->publish(QString("ch.%1").arg(c), Message::create("ch"));
                    }
                }
            });
        }

        for (auto& t : threads) t.join();

        for (int c = 0; c < numChannels; ++c) {
            QCOMPARE(counters[c].load(), 4 * 500);
        }
    }

    // 4. 快速 subscribe/unsubscribe 周期
    void testRapidSubscriptionCycle() {
        auto bus = std::make_shared<InMemoryMessageBus>();

        for (int cycle = 0; cycle < 50; ++cycle) {
            auto sub = bus->subscribe("rapid", [](const auto&) {});
            bus->publish("rapid", Message::create("rapid"));
            bus->unsubscribe(sub);
            bus->unsubscribeAll("rapid");
        }

        QCOMPARE(bus->subscriberCount("rapid"), 0);
    }
};

QTEST_MAIN(TestMessageBusStress)
#include "test_message_bus_stress.moc"
