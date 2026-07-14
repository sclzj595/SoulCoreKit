#include <QTest>
#include "soul/event/event_bus.h"

class TestEventBus : public QObject {
    Q_OBJECT

private slots:
    void testSubscribeAndPublish();
    void testUnsubscribe();
    void testSubscriberCount();
};

void TestEventBus::testSubscribeAndPublish() {
    sc::DefaultEventBus bus;
    int receivedValue = 0;

    auto subscription = bus.subscribe("test_topic", [&receivedValue](const std::string& data) {
        receivedValue = std::stoi(data);
    });

    bus.publish("test_topic", "42");
    QCOMPARE(receivedValue, 42);
}

void TestEventBus::testUnsubscribe() {
    sc::DefaultEventBus bus;
    int receivedCount = 0;

    auto sub = bus.subscribe("test_topic", [&receivedCount](const std::string&) {
        receivedCount++;
    });

    bus.publish("test_topic", "1");
    QCOMPARE(receivedCount, 1);

    sub.unsubscribe();
    bus.publish("test_topic", "2");
    QCOMPARE(receivedCount, 1);
}

void TestEventBus::testSubscriberCount() {
    sc::DefaultEventBus bus;

    QCOMPARE(bus.subscriberCount("test_topic"), 0u);

    auto sub1 = bus.subscribe("test_topic", [](const std::string&) {});
    QCOMPARE(bus.subscriberCount("test_topic"), 1u);

    auto sub2 = bus.subscribe("test_topic", [](const std::string&) {});
    QCOMPARE(bus.subscriberCount("test_topic"), 2u);

    sub1.unsubscribe();
    QCOMPARE(bus.subscriberCount("test_topic"), 1u);
}

QTEST_MAIN(TestEventBus)
#include "test_event_bus.moc"
