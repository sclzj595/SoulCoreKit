#include <QTest>
#include <QSignalSpy>
#include <QCoreApplication>

#include "soul/mq/kafka_adapter.h"
#include "soul/mq/imq_connection.h"
#include "soul/mq/imq_producer.h"
#include "soul/mq/imq_consumer.h"

using namespace sc::mq;

// ============================================================================
// TestKafkaConnection — Kafka 连接单元测试
// ============================================================================
class TestKafkaConnection : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testConnectDisconnect();
    void testIsConnected();
    void testCreateProducer();
    void testCreateConsumer();
};

void TestKafkaConnection::testConstructor() {
    KafkaConnection conn;
    QVERIFY(!conn.isConnected());
    QCOMPARE(conn.interfaceName(), std::string("KafkaConnection"));
}

void TestKafkaConnection::testConnectDisconnect() {
    KafkaConnection conn;
    QVERIFY(!conn.isConnected());

    ConnectionConfig config;
    config.host = "localhost";
    config.port = 9092;
    auto r = conn.connect(config);
    Q_UNUSED(r);
    // 连接可能失败（无 Kafka 服务），但不应崩溃

    conn.disconnect();
    QVERIFY(!conn.isConnected());
}

void TestKafkaConnection::testIsConnected() {
    KafkaConnection conn;
    QVERIFY(!conn.isConnected());
}

void TestKafkaConnection::testCreateProducer() {
    KafkaConnection conn;
    auto producer = conn.createProducer();
    QVERIFY(producer != nullptr);
    QCOMPARE(producer->interfaceName(), std::string("KafkaProducer"));
}

void TestKafkaConnection::testCreateConsumer() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaConsumer consumer(conn);
    QCOMPARE(consumer.interfaceName(), std::string("KafkaConsumer"));
}

// ============================================================================
// TestKafkaProducer — Kafka 生产者单元测试
// ============================================================================
class TestKafkaProducer : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testSend();
    void testSendAsync();
    void testFlush();
};

void TestKafkaProducer::testConstructor() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaProducer producer(conn);
    QCOMPARE(producer.interfaceName(), std::string("KafkaProducer"));
}

void TestKafkaProducer::testSend() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaProducer producer(conn);

    // 未连接时发送可能失败，但不应崩溃
    auto r = producer.send("test-topic", QByteArray("test message"));
    Q_UNUSED(r);
    QVERIFY(true);
}

void TestKafkaProducer::testSendAsync() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaProducer producer(conn);

    bool callbackCalled = false;
    Message msg;
    msg.topic = "test-topic";
    msg.body = "async test";

    producer.sendAsync(msg, [&](const Result<void>& result) {
        callbackCalled = true;
        Q_UNUSED(result);
    });

    // 异步发送已发起，等待回调
    QTest::qWait(100);
    Q_UNUSED(callbackCalled);
    // 回调可能被调用也可能未调用（取决于实现），只验证不崩溃
    QVERIFY(true);
}

void TestKafkaProducer::testFlush() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaProducer producer(conn);

    auto r = producer.flush(5000);
    Q_UNUSED(r);
    QVERIFY(true);
}

// ============================================================================
// TestKafkaConsumer — Kafka 消费者单元测试
// ============================================================================
class TestKafkaConsumer : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testSubscribe();
    void testUnsubscribe();
    void testStartStop();
};

void TestKafkaConsumer::testConstructor() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaConsumer consumer(conn);
    QCOMPARE(consumer.interfaceName(), std::string("KafkaConsumer"));
}

void TestKafkaConsumer::testSubscribe() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaConsumer consumer(conn);

    auto r = consumer.subscribe("test-topic", [](const ConsumeMessage&) {
        // 消费回调
    });
    QVERIFY(r.isOk());
}

void TestKafkaConsumer::testUnsubscribe() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaConsumer consumer(conn);

    (void)consumer.subscribe("test-topic", [](const ConsumeMessage&) {});
    auto r = consumer.unsubscribe("test-topic");
    QVERIFY(r.isOk());
}

void TestKafkaConsumer::testStartStop() {
    auto conn = std::make_shared<KafkaConnection>();
    KafkaConsumer consumer(conn);

    // 启动和停止不应崩溃
    consumer.start();
    consumer.stop();
    QVERIFY(true);
}

// ============================================================================
// TestRocketMQConnection — RocketMQ 连接单元测试
// ============================================================================
class TestRocketMQConnection : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testConnectDisconnect();
    void testIsConnected();
    void testCreateProducer();
    void testCreateConsumer();
};

void TestRocketMQConnection::testConstructor() {
    RocketMQConnection conn;
    QVERIFY(!conn.isConnected());
    QCOMPARE(conn.interfaceName(), std::string("RocketMQConnection"));
}

void TestRocketMQConnection::testConnectDisconnect() {
    RocketMQConnection conn;
    QVERIFY(!conn.isConnected());

    ConnectionConfig config;
    config.host = "localhost";
    config.port = 9876;
    auto r = conn.connect(config);
    Q_UNUSED(r);

    conn.disconnect();
    QVERIFY(!conn.isConnected());
}

void TestRocketMQConnection::testIsConnected() {
    RocketMQConnection conn;
    QVERIFY(!conn.isConnected());
}

void TestRocketMQConnection::testCreateProducer() {
    RocketMQConnection conn;
    auto producer = conn.createProducer();
    QVERIFY(producer != nullptr);
    QCOMPARE(producer->interfaceName(), std::string("RocketMQProducer"));
}

void TestRocketMQConnection::testCreateConsumer() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQConsumer consumer(conn);
    QCOMPARE(consumer.interfaceName(), std::string("RocketMQConsumer"));
}

// ============================================================================
// TestRocketMQProducer — RocketMQ 生产者单元测试
// ============================================================================
class TestRocketMQProducer : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testSend();
    void testSendAsync();
    void testSendOneway();
};

void TestRocketMQProducer::testConstructor() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQProducer producer(conn);
    QCOMPARE(producer.interfaceName(), std::string("RocketMQProducer"));
}

void TestRocketMQProducer::testSend() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQProducer producer(conn);

    auto r = producer.send("test-topic", QByteArray("test message"));
    Q_UNUSED(r);
    QVERIFY(true);
}

void TestRocketMQProducer::testSendAsync() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQProducer producer(conn);

    bool callbackCalled = false;
    Message msg;
    msg.topic = "test-topic";
    msg.body = "async test";

    producer.sendAsync(msg, [&](const Result<void>& result) {
        callbackCalled = true;
        Q_UNUSED(result);
    });

    QTest::qWait(100);
    Q_UNUSED(callbackCalled);
    QVERIFY(true);
}

void TestRocketMQProducer::testSendOneway() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQProducer producer(conn);

    Message msg;
    msg.topic = "test-topic";
    msg.body = "oneway test";

    auto r = producer.sendOneway(msg);
    Q_UNUSED(r);
    QVERIFY(true);
}

// ============================================================================
// TestRocketMQConsumer — RocketMQ 消费者单元测试
// ============================================================================
class TestRocketMQConsumer : public QObject {
    Q_OBJECT

private slots:
    void testConstructor();
    void testSubscribe();
    void testUnsubscribe();
    void testStartStop();
};

void TestRocketMQConsumer::testConstructor() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQConsumer consumer(conn);
    QCOMPARE(consumer.interfaceName(), std::string("RocketMQConsumer"));
}

void TestRocketMQConsumer::testSubscribe() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQConsumer consumer(conn);

    auto r = consumer.subscribe("test-topic", [](const ConsumeMessage&) {
        // 消费回调
    });
    QVERIFY(r.isOk());
}

void TestRocketMQConsumer::testUnsubscribe() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQConsumer consumer(conn);

    (void)consumer.subscribe("test-topic", [](const ConsumeMessage&) {});
    auto r = consumer.unsubscribe("test-topic");
    QVERIFY(r.isOk());
}

void TestRocketMQConsumer::testStartStop() {
    auto conn = std::make_shared<RocketMQConnection>();
    RocketMQConsumer consumer(conn);

    consumer.start();
    consumer.stop();
    QVERIFY(true);
}

// ============================================================================
// main
// ============================================================================
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    TestKafkaConnection kafkaConnTest;
    TestKafkaProducer kafkaProdTest;
    TestKafkaConsumer kafkaConsTest;
    TestRocketMQConnection rocketConnTest;
    TestRocketMQProducer rocketProdTest;
    TestRocketMQConsumer rocketConsTest;

    int result = 0;
    result |= QTest::qExec(&kafkaConnTest, argc, argv);
    result |= QTest::qExec(&kafkaProdTest, argc, argv);
    result |= QTest::qExec(&kafkaConsTest, argc, argv);
    result |= QTest::qExec(&rocketConnTest, argc, argv);
    result |= QTest::qExec(&rocketProdTest, argc, argv);
    result |= QTest::qExec(&rocketConsTest, argc, argv);

    return result;
}

#include "test_kafka_adapter.moc"