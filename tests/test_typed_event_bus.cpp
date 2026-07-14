#include <QTest>
#include "soul/event/typed_event_bus.h"
#include "soul/event/event_bus.h"
#include <QThread>

class TestTypedEventBus : public QObject {
    Q_OBJECT

private slots:
    void testTypeSafePublishSubscribe();
    void testMultipleTypesCoexist();
    void testRAIIAutoUnsubscribe();
    void testAsyncDispatch();
    void testSubscriberCount();
    void testClear();
    void testMoveSemantics();
};

struct UserEvent {
    int userId;
    QString userName;
};

struct MessageEvent {
    QString sender;
    QString content;
};

void TestTypedEventBus::testTypeSafePublishSubscribe() {
    auto bus = sc::TypedEventBus::create();
    UserEvent receivedEvent;
    int receivedCount = 0;

    auto sub = bus->subscribe<UserEvent>("user.login", [&receivedEvent, &receivedCount](const UserEvent& event) {
        receivedEvent = event;
        receivedCount++;
    });

    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 1u);

    bus->publish("user.login", UserEvent{123, "Alice"});

    QCOMPARE(receivedCount, 1);
    QCOMPARE(receivedEvent.userId, 123);
    QCOMPARE(receivedEvent.userName, QString("Alice"));
}

void TestTypedEventBus::testMultipleTypesCoexist() {
    auto bus = sc::TypedEventBus::create();
    int userEventCount = 0;
    int messageEventCount = 0;

    auto sub1 = bus->subscribe<UserEvent>("user.login", [&userEventCount](const UserEvent&) {
        userEventCount++;
    });

    auto sub2 = bus->subscribe<MessageEvent>("message.receive", [&messageEventCount](const MessageEvent&) {
        messageEventCount++;
    });

    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 1u);
    QCOMPARE(bus->subscriberCount<MessageEvent>("message.receive"), 1u);

    bus->publish("user.login", UserEvent{1, "Test"});
    bus->publish("message.receive", MessageEvent{"Bob", "Hello"});
    bus->publish("user.login", UserEvent{2, "Test2"});

    QCOMPARE(userEventCount, 2);
    QCOMPARE(messageEventCount, 1);

    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 1u);
    QCOMPARE(bus->subscriberCount<MessageEvent>("message.receive"), 1u);
}

void TestTypedEventBus::testRAIIAutoUnsubscribe() {
    auto bus = sc::TypedEventBus::create();
    int receivedCount = 0;

    {
        auto sub = bus->subscribe<UserEvent>("user.login", [&receivedCount](const UserEvent&) {
            receivedCount++;
        });
        QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 1u);

        bus->publish("user.login", UserEvent{1, "Test"});
        QCOMPARE(receivedCount, 1);
    }

    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 0u);

    bus->publish("user.login", UserEvent{2, "Test"});
    QCOMPARE(receivedCount, 1);
}

void TestTypedEventBus::testAsyncDispatch() {
    auto bus = sc::TypedEventBus::create();
    int receivedCount = 0;
    bool asyncCompleted = false;

    auto sub = bus->subscribe<UserEvent>("user.login", [&receivedCount, &asyncCompleted](const UserEvent&) {
        receivedCount++;
        asyncCompleted = true;
    }, sc::DispatchPolicy::Async);

    bus->publish("user.login", UserEvent{1, "AsyncTest"});

    QElapsedTimer timer;
    timer.start();
    while (!asyncCompleted && timer.elapsed() < 2000) {
        QCoreApplication::processEvents();
        QThread::msleep(20);
    }

    QVERIFY(asyncCompleted);
    QCOMPARE(receivedCount, 1);
}

void TestTypedEventBus::testSubscriberCount() {
    auto bus = sc::TypedEventBus::create();

    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 0u);

    auto sub1 = bus->subscribe<UserEvent>("user.login", [](const UserEvent&) {});
    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 1u);

    auto sub2 = bus->subscribe<UserEvent>("user.login", [](const UserEvent&) {});
    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 2u);

    auto sub3 = bus->subscribe<MessageEvent>("message.receive", [](const MessageEvent&) {});
    QCOMPARE(bus->subscriberCount<MessageEvent>("message.receive"), 1u);

    QCOMPARE(bus->totalSubscriberCount(), 3u);

    sub1.unsubscribe();
    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 1u);

    sub2.unsubscribe();
    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 0u);

    QCOMPARE(bus->totalSubscriberCount(), 1u);
}

void TestTypedEventBus::testClear() {
    auto bus = sc::TypedEventBus::create();

    auto sub1 = bus->subscribe<UserEvent>("user.login", [](const UserEvent&) {});
    auto sub2 = bus->subscribe<MessageEvent>("message.receive", [](const MessageEvent&) {});

    QCOMPARE(bus->totalSubscriberCount(), 2u);

    bus->clear();

    QCOMPARE(bus->totalSubscriberCount(), 0u);
    QCOMPARE(bus->subscriberCount<UserEvent>("user.login"), 0u);
    QCOMPARE(bus->subscriberCount<MessageEvent>("message.receive"), 0u);
}

void TestTypedEventBus::testMoveSemantics() {
    auto bus = sc::TypedEventBus::create();
    int receivedCount = 0;

    sc::Subscription sub1 = bus->subscribe<UserEvent>("user.login", [&receivedCount](const UserEvent&) {
        receivedCount++;
    });

    sc::Subscription sub2 = std::move(sub1);

    QVERIFY(!sub1.isValid());
    QVERIFY(sub2.isValid());

    bus->publish("user.login", UserEvent{1, "Test"});
    QCOMPARE(receivedCount, 1);

    sc::Subscription sub3;
    sub3 = std::move(sub2);

    QVERIFY(!sub2.isValid());
    QVERIFY(sub3.isValid());

    bus->publish("user.login", UserEvent{2, "Test2"});
    QCOMPARE(receivedCount, 2);
}

QTEST_MAIN(TestTypedEventBus)
#include "test_typed_event_bus.moc"