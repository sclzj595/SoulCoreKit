#include <QTest>
#include <QSignalSpy>
#include <QByteArray>
#include <QString>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "soul/mq/iamqp_backend.h"
#include "soul/mq/inmemory_amqp_backend.h"
#include "soul/mq/rabbitmq/rabbitmq_connection.h"
#include "soul/mq/rabbitmq/rabbitmq_producer.h"
#include "soul/mq/rabbitmq/rabbitmq_consumer.h"
#include "soul/mq/mq_factory.h"

using namespace sc;
using namespace sc::mq;

// ============================================================================
// TestInMemoryAmqpBackend — 内存 AMQP 后端单元测试
// ============================================================================
class TestInMemoryAmqpBackend : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void testConnectDisconnect();
    void testDeclareExchange();
    void testDeclareQueue();
    void testBindUnbindQueue();
    void testDirectExchangeRouting();
    void testFanoutExchangeRouting();
    void testTopicExchangeStarPattern();
    void testTopicExchangeHashPattern();
    void testConsumeAndAck();
    void testConsumeAndNackRequeue();
    void testPrefetchCount();
    void testMultipleQueuesSingleConsumer();
    void testCancelConsume();
    void testCancelConsumeInCallback();
    void testTopicPatternMatching_data();
    void testTopicPatternMatching();
};

void TestInMemoryAmqpBackend::initTestCase() {
}

void TestInMemoryAmqpBackend::cleanupTestCase() {
}

void TestInMemoryAmqpBackend::testConnectDisconnect() {
    InMemoryAmqpBackend backend;
    QVERIFY(!backend.isConnected());

    auto r = backend.connect({});
    QVERIFY(r.isOk());
    QVERIFY(backend.isConnected());

    backend.disconnect();
    QVERIFY(!backend.isConnected());
}

void TestInMemoryAmqpBackend::testDeclareExchange() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    QVERIFY(backend.declareExchange("ex1", ExchangeType::Direct).isOk());
    QVERIFY(backend.declareExchange("ex2", ExchangeType::Fanout).isOk());
    QVERIFY(backend.declareExchange("ex3", ExchangeType::Topic).isOk());

    QVERIFY(backend.declareExchange("ex1", ExchangeType::Direct).isOk());

    auto r = backend.declareExchange("", ExchangeType::Direct);
    QVERIFY(!r.isOk());
    QCOMPARE(r.unwrapErr().code(), ErrorCode::InvalidArgument);
}

void TestInMemoryAmqpBackend::testDeclareQueue() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    auto r1 = backend.declareQueue("q1");
    QVERIFY(r1.isOk());
    QCOMPARE(r1.unwrap(), QString("q1"));

    auto r2 = backend.declareQueue("");
    QVERIFY(r2.isOk());
    QVERIFY(!r2.unwrap().isEmpty());
    QVERIFY(r2.unwrap().startsWith("amq.gen-"));

    auto r3 = backend.declareQueue("q1");
    QVERIFY(r3.isOk());
}

void TestInMemoryAmqpBackend::testBindUnbindQueue() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");

    QVERIFY(backend.bindQueue("q1", "ex", "key1").isOk());

    QVERIFY(backend.unbindQueue("q1", "ex", "key1").isOk());

    auto r = backend.unbindQueue("q1", "ex", "key1");
    QVERIFY(!r.isOk());
    QCOMPARE(r.unwrapErr().code(), ErrorCode::NotFound);
}

void TestInMemoryAmqpBackend::testDirectExchangeRouting() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("direct_ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.declareQueue("q2");
    (void)backend.bindQueue("q1", "direct_ex", "key1");
    (void)backend.bindQueue("q2", "direct_ex", "key2");

    std::vector<AmqpDelivery> received;
    std::mutex mtx;

    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        std::lock_guard<std::mutex> lock(mtx);
        received.push_back(d);
        (void)backend.ack(d.deliveryTag);
    });
    (void)backend.consume("q2", [&](const AmqpDelivery& d) {
        std::lock_guard<std::mutex> lock(mtx);
        received.push_back(d);
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    AmqpMessage msg1;
    msg1.exchange = "direct_ex";
    msg1.routingKey = "key1";
    msg1.body = "hello1";
    QVERIFY(backend.publish(msg1).isOk());

    AmqpMessage msg2;
    msg2.exchange = "direct_ex";
    msg2.routingKey = "key2";
    msg2.body = "hello2";
    QVERIFY(backend.publish(msg2).isOk());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    backend.stopConsuming();

    QCOMPARE(received.size(), static_cast<std::size_t>(2));
}

void TestInMemoryAmqpBackend::testFanoutExchangeRouting() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("fanout_ex", ExchangeType::Fanout);
    (void)backend.declareQueue("q1");
    (void)backend.declareQueue("q2");
    (void)backend.declareQueue("q3");
    (void)backend.bindQueue("q1", "fanout_ex", "");
    (void)backend.bindQueue("q2", "fanout_ex", "");
    (void)backend.bindQueue("q3", "fanout_ex", "");

    std::atomic<int> count{0};

    auto cb = [&](const AmqpDelivery& d) {
        (void)d;
        count++;
        (void)backend.ack(d.deliveryTag);
    };

    (void)backend.consume("q1", cb);
    (void)backend.consume("q2", cb);
    (void)backend.consume("q3", cb);

    backend.startConsuming();

    AmqpMessage msg;
    msg.exchange = "fanout_ex";
    msg.routingKey = "";
    msg.body = "broadcast";
    QVERIFY(backend.publish(msg).isOk());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    backend.stopConsuming();

    QCOMPARE(count.load(), 3);
}

void TestInMemoryAmqpBackend::testTopicExchangeStarPattern() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("topic_ex", ExchangeType::Topic);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "topic_ex", "logs.*.error");

    std::atomic<int> count{0};

    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        (void)d;
        count++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    AmqpMessage msg1;
    msg1.exchange = "topic_ex";
    msg1.routingKey = "logs.app.error";
    msg1.body = "match1";
    (void)backend.publish(msg1);

    AmqpMessage msg2;
    msg2.exchange = "topic_ex";
    msg2.routingKey = "logs.app.db.error";
    msg2.body = "no_match";
    (void)backend.publish(msg2);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    backend.stopConsuming();

    QCOMPARE(count.load(), 1);
}

void TestInMemoryAmqpBackend::testTopicExchangeHashPattern() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("topic_ex", ExchangeType::Topic);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "topic_ex", "logs.#");

    std::atomic<int> count{0};

    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        (void)d;
        count++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    AmqpMessage msg1;
    msg1.exchange = "topic_ex";
    msg1.routingKey = "logs";
    msg1.body = "match";
    (void)backend.publish(msg1);

    AmqpMessage msg2;
    msg2.exchange = "topic_ex";
    msg2.routingKey = "logs.app.error";
    msg2.body = "match";
    (void)backend.publish(msg2);

    AmqpMessage msg3;
    msg3.exchange = "topic_ex";
    msg3.routingKey = "logs.app.db.critical";
    msg3.body = "match";
    (void)backend.publish(msg3);

    AmqpMessage msg4;
    msg4.exchange = "topic_ex";
    msg4.routingKey = "metrics.app.error";
    msg4.body = "no_match";
    (void)backend.publish(msg4);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    backend.stopConsuming();

    QCOMPARE(count.load(), 3);
}

void TestInMemoryAmqpBackend::testConsumeAndAck() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "ex", "key");

    qint64 receivedTag = 0;
    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        receivedTag = d.deliveryTag;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    AmqpMessage msg;
    msg.exchange = "ex";
    msg.routingKey = "key";
    msg.body = "test";
    (void)backend.publish(msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    backend.stopConsuming();

    QVERIFY(receivedTag > 0);

    auto r = backend.ack(receivedTag);
    QVERIFY(!r.isOk());
    QCOMPARE(r.unwrapErr().code(), ErrorCode::NotFound);
}

void TestInMemoryAmqpBackend::testConsumeAndNackRequeue() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "ex", "key");

    std::atomic<int> count{0};
    qint64 firstTag = 0;

    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        count++;
        if (count == 1) {
            firstTag = d.deliveryTag;
            (void)backend.nack(d.deliveryTag, true);
        } else {
            (void)backend.ack(d.deliveryTag);
        }
    });

    backend.startConsuming();

    AmqpMessage msg;
    msg.exchange = "ex";
    msg.routingKey = "key";
    msg.body = "test";
    (void)backend.publish(msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backend.stopConsuming();

    QCOMPARE(count.load(), 2);
}

void TestInMemoryAmqpBackend::testPrefetchCount() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "ex", "key");

    std::atomic<int> received{0};

    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        (void)d;
        received++;
    }, 2);

    backend.startConsuming();

    for (int i = 0; i < 5; ++i) {
        AmqpMessage msg;
        msg.exchange = "ex";
        msg.routingKey = "key";
        msg.body = QString::number(i).toUtf8();
        (void)backend.publish(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backend.stopConsuming();

    QCOMPARE(received.load(), 2);
}

void TestInMemoryAmqpBackend::testMultipleQueuesSingleConsumer() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.declareQueue("q2");
    (void)backend.bindQueue("q1", "ex", "key1");
    (void)backend.bindQueue("q2", "ex", "key2");

    std::atomic<int> q1_count{0};
    std::atomic<int> q2_count{0};

    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        q1_count++;
        (void)backend.ack(d.deliveryTag);
    });
    (void)backend.consume("q2", [&](const AmqpDelivery& d) {
        q2_count++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    for (int i = 0; i < 3; ++i) {
        AmqpMessage msg1;
        msg1.exchange = "ex";
        msg1.routingKey = "key1";
        msg1.body = QByteArray::number(i);
        (void)backend.publish(msg1);

        AmqpMessage msg2;
        msg2.exchange = "ex";
        msg2.routingKey = "key2";
        msg2.body = QByteArray::number(i);
        (void)backend.publish(msg2);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    backend.stopConsuming();

    QCOMPARE(q1_count.load(), 3);
    QCOMPARE(q2_count.load(), 3);
}

void TestInMemoryAmqpBackend::testCancelConsume() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareQueue("q1");

    std::atomic<int> count{0};
    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        (void)d;
        count++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    QVERIFY(backend.cancelConsume("q1").isOk());

    auto r = backend.cancelConsume("nonexistent");
    QVERIFY(r.isOk());

    backend.stopConsuming();
}

void TestInMemoryAmqpBackend::testCancelConsumeInCallback() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "ex", "key");

    std::atomic<int> count{0};
    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        count++;
        (void)backend.cancelConsume("q1");
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    for (int i = 0; i < 3; ++i) {
        AmqpMessage msg;
        msg.exchange = "ex";
        msg.routingKey = "key";
        msg.body = QByteArray::number(i);
        (void)backend.publish(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backend.stopConsuming();

    QCOMPARE(count.load(), 1);
}

void TestInMemoryAmqpBackend::testTopicPatternMatching_data() {
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("key");
    QTest::addColumn<bool>("expected");

    QTest::newRow("exact_match") << "logs.error" << "logs.error" << true;
    QTest::newRow("exact_no_match") << "logs.error" << "logs.info" << false;
    QTest::newRow("star_match") << "logs.*.error" << "logs.app.error" << true;
    QTest::newRow("star_no_match_extra") << "logs.*.error" << "logs.app.db.error" << false;
    QTest::newRow("star_no_match_less") << "logs.*.error" << "logs.error" << false;
    QTest::newRow("hash_match_zero") << "logs.#" << "logs" << true;
    QTest::newRow("hash_match_one") << "logs.#" << "logs.app" << true;
    QTest::newRow("hash_match_many") << "logs.#" << "logs.app.db.critical" << true;
    QTest::newRow("hash_no_match") << "logs.#" << "metrics.app" << false;
    QTest::newRow("combo_match") << "*.app.#" << "logs.app.error.critical" << true;
    QTest::newRow("combo_no_match") << "*.app.#" << "logs.system.error" << false;
}

void TestInMemoryAmqpBackend::testTopicPatternMatching() {
    QFETCH(QString, pattern);
    QFETCH(QString, key);
    QFETCH(bool, expected);

    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("topic_ex", ExchangeType::Topic);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "topic_ex", pattern);

    std::atomic<bool> received{false};
    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        (void)d;
        received = true;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    AmqpMessage msg;
    msg.exchange = "topic_ex";
    msg.routingKey = key;
    msg.body = "test";
    (void)backend.publish(msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    backend.stopConsuming();

    QCOMPARE(received.load(), expected);
}

// ============================================================================
// TestRabbitMQConnection — RabbitMQ 连接测试
// ============================================================================
class TestRabbitMQConnection : public QObject {
    Q_OBJECT

private slots:
    void testConnectDisconnect();
    void testProducerConsumer();
    void testFactoryCreation();
};

void TestRabbitMQConnection::testConnectDisconnect() {
    auto conn = std::make_shared<RabbitMQConnection>();
    conn->setBackendType(RabbitMQConnection::BackendType::InMemory);

    QVERIFY(!conn->isConnected());

    ConnectionConfig config;
    config.host = "localhost";
    auto r = conn->connect(config);
    QVERIFY(r.isOk());
    QVERIFY(conn->isConnected());

    conn->disconnect();
    QVERIFY(!conn->isConnected());
}

void TestRabbitMQConnection::testProducerConsumer() {
    auto conn = std::make_shared<RabbitMQConnection>();
    conn->setBackendType(RabbitMQConnection::BackendType::InMemory);
    (void)conn->connect({});

    auto producer = conn->createProducer();
    auto consumer = conn->createConsumer();

    QVERIFY(producer != nullptr);
    QVERIFY(consumer != nullptr);

    auto rmqProducer = std::dynamic_pointer_cast<RabbitMQProducer>(producer);
    QVERIFY(rmqProducer != nullptr);

    QVERIFY(rmqProducer->declareExchange("test_ex", "direct").isOk());
    QVERIFY(rmqProducer->bindQueue("test_q", "test_ex", "test_key").isOk());

    std::atomic<bool> received{false};
    QByteArray receivedBody;

    QVERIFY(consumer->subscribe("test_ex", "test_q", [&](const ConsumeMessage& msg) {
        received = true;
        receivedBody = msg.body;
    }).isOk());

    consumer->start();

    QVERIFY(producer->send("test_ex", "test_key", "hello world").isOk());

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    consumer->stop();
    conn->disconnect();

    QVERIFY(received.load());
    QCOMPARE(receivedBody, QByteArray("hello world"));
}

void TestRabbitMQConnection::testFactoryCreation() {
    auto conn = MQFactory::createConnection(MQType::RabbitMQ);
    QVERIFY(conn != nullptr);

    auto conn2 = MQFactory::createConnection(MQType::Kafka);
    QCOMPARE(conn2, nullptr);
}

// ============================================================================
// TestMQAdvanced — v1.9.2 真实消息生产/消费场景 [P1-M06]
// ============================================================================
class TestMQAdvanced : public QObject {
    Q_OBJECT

private slots:
    void testHighVolumePublish();
    void testMultipleProducers();
    void testMessageRejection();
    void testConsumerDrain();
    void testPublishAfterDisconnect();
};

void TestMQAdvanced::testHighVolumePublish() {
    // 验证: 大量消息发布与消费无丢失
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("high_vol_ex", ExchangeType::Direct);
    (void)backend.declareQueue("hv_q");
    (void)backend.bindQueue("hv_q", "high_vol_ex", "key");

    std::atomic<int> received{0};
    (void)backend.consume("hv_q", [&](const AmqpDelivery& d) {
        received++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    const int msgCount = 100;
    for (int i = 0; i < msgCount; ++i) {
        AmqpMessage msg;
        msg.exchange = "high_vol_ex";
        msg.routingKey = "key";
        msg.body = QByteArray::number(i);
        QVERIFY(backend.publish(msg).isOk());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    backend.stopConsuming();

    QCOMPARE(received.load(), msgCount);
}

void TestMQAdvanced::testMultipleProducers() {
    // 验证: 多个生产者向同一队列发消息,消费者正确接收
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("multi_ex", ExchangeType::Direct);
    (void)backend.declareQueue("multi_q");
    (void)backend.bindQueue("multi_q", "multi_ex", "multi_key");

    std::atomic<int> received{0};
    (void)backend.consume("multi_q", [&](const AmqpDelivery& d) {
        received++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    // 模拟 5 个生产者
    std::vector<std::thread> producers;
    for (int p = 0; p < 5; ++p) {
        producers.emplace_back([&backend, p]() {
            for (int i = 0; i < 10; ++i) {
                AmqpMessage msg;
                msg.exchange = "multi_ex";
                msg.routingKey = "multi_key";
                msg.body = QByteArray::number(p * 100 + i);
                (void)backend.publish(msg);
            }
        });
    }

    for (auto& t : producers) {
        t.join();
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    backend.stopConsuming();

    QCOMPARE(received.load(), 50);
}

void TestMQAdvanced::testMessageRejection() {
    // 验证: nack(requeue=false) 后消息被丢弃,不再投递
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("reject_ex", ExchangeType::Direct);
    (void)backend.declareQueue("reject_q");
    (void)backend.bindQueue("reject_q", "reject_ex", "key");

    std::atomic<int> received{0};
    (void)backend.consume("reject_q", [&](const AmqpDelivery& d) {
        received++;
        // nack 不重新入队 — 消息被丢弃
        (void)backend.nack(d.deliveryTag, false);
    });

    backend.startConsuming();

    AmqpMessage msg;
    msg.exchange = "reject_ex";
    msg.routingKey = "key";
    msg.body = "reject me";
    (void)backend.publish(msg);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backend.stopConsuming();

    // 只收到一次(nack 后丢弃)
    QCOMPARE(received.load(), 1);
}

void TestMQAdvanced::testConsumerDrain() {
    // 验证: 先发布后消费 — 消息被缓冲,消费者启动后收到
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("drain_ex", ExchangeType::Direct);
    (void)backend.declareQueue("drain_q");
    (void)backend.bindQueue("drain_q", "drain_ex", "key");

    // 先发布消息(消费者尚未启动)
    for (int i = 0; i < 5; ++i) {
        AmqpMessage msg;
        msg.exchange = "drain_ex";
        msg.routingKey = "key";
        msg.body = QByteArray::number(i);
        QVERIFY(backend.publish(msg).isOk());
    }

    // 后订阅
    std::atomic<int> received{0};
    (void)backend.consume("drain_q", [&](const AmqpDelivery& d) {
        received++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backend.stopConsuming();

    // 应该收到积压的 5 条消息
    QCOMPARE(received.load(), 5);
}

void TestMQAdvanced::testPublishAfterDisconnect() {
    // 验证: 断开连接后 publish 返回错误
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("disc_ex", ExchangeType::Direct);
    (void)backend.declareQueue("disc_q");
    (void)backend.bindQueue("disc_q", "disc_ex", "key");

    backend.disconnect();
    QVERIFY(!backend.isConnected());

    AmqpMessage msg;
    msg.exchange = "disc_ex";
    msg.routingKey = "key";
    msg.body = "after disconnect";
    auto r = backend.publish(msg);
    QVERIFY(!r.isOk());
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    
    TestInMemoryAmqpBackend amqpTest;
    TestRabbitMQConnection rmqTest;
    TestMQAdvanced advancedTest;
    
    int result = 0;
    result |= QTest::qExec(&amqpTest, argc, argv);
    result |= QTest::qExec(&rmqTest, argc, argv);
    result |= QTest::qExec(&advancedTest, argc, argv);
    
    return result;
}

#include "test_mq.moc"