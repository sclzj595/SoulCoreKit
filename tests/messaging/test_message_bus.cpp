// ============================================================================
// test_message_bus.cpp — InMemoryMessageBus 单元测试 [v2.9.1]
// ============================================================================

#include <QtTest>
#include <QThread>
#include <atomic>
#include <memory>
#include "soul/event/inmemory_message_bus.h"

using namespace sc;

class TestMessageBus : public QObject {
    Q_OBJECT

private slots:
    // 1. Message 构造
    void testMessageCreate() {
        auto msg = Message::create("test.topic", QByteArray("hello"));
        QVERIFY(!msg.id.isEmpty());
        QCOMPARE(msg.topic, QString("test.topic"));
        QCOMPARE(msg.payload, QByteArray("hello"));
        QVERIFY(msg.timestamp.isValid());
    }

    void testMessageFromJson() {
        auto msg = Message::fromJson("user.created", R"({"id":42})");
        QCOMPARE(msg.contentType, QString("application/json"));
        QCOMPARE(msg.payloadString(), QString(R"({"id":42})"));
    }

    void testMessageInheritContext() {
        auto msg = Message::create("test");
        // 无 Context 时安全
        msg.inheritContext();
        QVERIFY(msg.traceId.isEmpty());
    }

    // 2. 基本 publish / subscribe
    void testBasicPubSub() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};

        auto sub = bus->subscribe("test", [&received](const std::shared_ptr<void>&) {
            received.fetch_add(1);
        });

        QVERIFY(bus->hasSubscribers("test"));
        QCOMPARE(bus->subscriberCount("test"), 1);
        QCOMPARE(bus->totalSubscriberCount(), 1);

        bus->publish("test", Message::create("test"));
        QCOMPARE(received.load(), 1);
    }

    // 3. 多订阅者
    void testMultipleSubscribers() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> count{0};

        auto s1 = bus->subscribe("multi", [&count](const auto&) { count++; });
        auto s2 = bus->subscribe("multi", [&count](const auto&) { count++; });
        auto s3 = bus->subscribe("multi", [&count](const auto&) { count++; });

        bus->publish("multi", Message::create("multi"));
        QCOMPARE(count.load(), 3);
        QCOMPARE(bus->subscriberCount("multi"), 3);
    }

    // 4. 取消订阅
    void testUnsubscribe() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};

        auto sub = bus->subscribe("test", [&received](const auto&) { received++; });
        bus->publish("test", Message::create("test"));
        QCOMPARE(received.load(), 1);

        bus->unsubscribe(sub);
        bus->publish("test", Message::create("test"));
        QCOMPARE(received.load(), 1);  // 不再收到
        QCOMPARE(bus->subscriberCount("test"), 0);
    }

    // 5. Subscription 析构自动取消
    void testSubscriptionAutoUnsubscribe() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};

        {
            auto sub = bus->subscribe("temp", [&received](const auto&) { received++; });
            bus->publish("temp", Message::create("temp"));
            QCOMPARE(received.load(), 1);
            QCOMPARE(bus->subscriberCount("temp"), 1);
        }
        // sub 离开作用域

        bus->publish("temp", Message::create("temp"));
        QCOMPARE(received.load(), 1);  // 不再收到
    }

    // 6. unsubscribeAll
    void testUnsubscribeAll() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> a{0}, b{0};

        // v3.0.0: 必须保存订阅句柄 (RAII 设计, 丢弃句柄会导致订阅立即析构)
        auto sa1 = bus->subscribe("ch1", [&a](const auto&) { a++; });
        auto sa2 = bus->subscribe("ch1", [&a](const auto&) { a++; });
        auto sb  = bus->subscribe("ch2", [&b](const auto&) { b++; });

        bus->unsubscribeAll("ch1");
        bus->publish("ch1", Message::create("ch1"));
        bus->publish("ch2", Message::create("ch2"));

        QCOMPARE(a.load(), 0);
        QCOMPARE(b.load(), 1);
        QCOMPARE(bus->subscriberCount("ch1"), 0);
    }

    // 7. shutdown 后不投递
    void testShutdown() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> received{0};

        bus->subscribe("test", [&received](const auto&) { received++; });
        bus->shutdown();

        bus->publish("test", Message::create("test"));
        QCOMPARE(received.load(), 0);
        QVERIFY(bus->isShutdown());
    }

    // 8. hasSubscribers / subscriberCount
    void testQueryMethods() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        QVERIFY(!bus->hasSubscribers("empty"));
        QCOMPARE(bus->subscriberCount("empty"), 0);

        auto s1 = bus->subscribe("q", [](const auto&) {});
        QVERIFY(bus->hasSubscribers("q"));
        QCOMPARE(bus->subscriberCount("q"), 1);
        QCOMPARE(bus->totalSubscriberCount(), 1);
    }

    // 9. Message with payload
    void testMessagePayload() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        QString result;

        // v3.0.0: 保存订阅句柄 (RAII)
        auto sub = bus->subscribe("data", [&result](const std::shared_ptr<void>& ptr) {
            auto msg = static_cast<const Message*>(ptr.get());
            result = msg->payloadString();
        });

        auto msg = Message::create("data", QByteArray("payload_value"));
        bus->publish("data", msg);
        QCOMPARE(result, QString("payload_value"));
    }

    // 10. 多频道隔离
    void testChannelIsolation() {
        auto bus = std::make_shared<InMemoryMessageBus>();
        std::atomic<int> ch1{0}, ch2{0};

        // v3.0.0: 保存订阅句柄 (RAII)
        auto s1 = bus->subscribe("ch1", [&ch1](const auto&) { ch1++; });
        auto s2 = bus->subscribe("ch2", [&ch2](const auto&) { ch2++; });

        bus->publish("ch1", Message::create("ch1"));
        QCOMPARE(ch1.load(), 1);
        QCOMPARE(ch2.load(), 0);

        bus->publish("ch2", Message::create("ch2"));
        QCOMPARE(ch1.load(), 1);
        QCOMPARE(ch2.load(), 1);
    }
};

QTEST_MAIN(TestMessageBus)
#include "test_message_bus.moc"
