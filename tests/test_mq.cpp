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

/**
 * @class TestInMemoryAmqpBackend
 * @brief IAmqpBackend/InMemoryAmqpBackend 单元测试
 *
 * 覆盖:
 * - 连接/断开
 * - Exchange 声明(Direct/Fanout/Topic)
 * - 队列声明/绑定/解绑
 * - 消息发布与路由(Direct/Fanout/Topic 通配符)
 * - 消费者订阅与消息分发
 * - QoS 预取计数
 * - 消息确认(ack/nack/reject)与重新入队
 */
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
    void testMultipleQueuesSingleConsumer();  // 多队列单消费者场景
    void testCancelConsume();
    void testCancelConsumeInCallback();  // 回归测试: 回调中 cancelConsume 不应 UAF
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

    // 正常声明
    QVERIFY(backend.declareExchange("ex1", ExchangeType::Direct).isOk());
    QVERIFY(backend.declareExchange("ex2", ExchangeType::Fanout).isOk());
    QVERIFY(backend.declareExchange("ex3", ExchangeType::Topic).isOk());

    // 幂等声明(同名重复声明返回成功)
    QVERIFY(backend.declareExchange("ex1", ExchangeType::Direct).isOk());

    // 空名称拒绝
    auto r = backend.declareExchange("", ExchangeType::Direct);
    QVERIFY(!r.isOk());
    QCOMPARE(r.unwrapErr().code(), ErrorCode::InvalidArgument);
}

void TestInMemoryAmqpBackend::testDeclareQueue() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    // 指定名称
    auto r1 = backend.declareQueue("q1");
    QVERIFY(r1.isOk());
    QCOMPARE(r1.unwrap(), QString("q1"));

    // 自动生成名称
    auto r2 = backend.declareQueue("");
    QVERIFY(r2.isOk());
    QVERIFY(!r2.unwrap().isEmpty());
    QVERIFY(r2.unwrap().startsWith("amq.gen-"));

    // 幂等声明
    auto r3 = backend.declareQueue("q1");
    QVERIFY(r3.isOk());
}

void TestInMemoryAmqpBackend::testBindUnbindQueue() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");

    // 绑定
    QVERIFY(backend.bindQueue("q1", "ex", "key1").isOk());

    // 解绑
    QVERIFY(backend.unbindQueue("q1", "ex", "key1").isOk());

    // 重复解绑失败
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

    // 等待分发
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

    // Fanout 应该投递到所有 3 个队列
    QCOMPARE(count.load(), 3);
}

void TestInMemoryAmqpBackend::testTopicExchangeStarPattern() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("topic_ex", ExchangeType::Topic);
    (void)backend.declareQueue("q1");
    // '*' 匹配一个词
    (void)backend.bindQueue("q1", "topic_ex", "logs.*.error");

    std::atomic<int> count{0};

    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        (void)d;
        count++;
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    // 匹配
    AmqpMessage msg1;
    msg1.exchange = "topic_ex";
    msg1.routingKey = "logs.app.error";
    msg1.body = "match1";
    (void)backend.publish(msg1);

    // 不匹配(两个词在 * 位置)
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
    // '#' 匹配零个或多个词
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

    // 不匹配(不以 logs 开头)
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

    // ack 后再次 ack 应该失败(已确认)
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
            // 第一次:nack 并重新入队
            (void)backend.nack(d.deliveryTag, true);
        } else {
            // 第二次:ack
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

    // 应该收到 2 次(第一次 nack requeue,第二次 ack)
    QCOMPARE(count.load(), 2);
}

void TestInMemoryAmqpBackend::testPrefetchCount() {
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "ex", "key");

    std::atomic<int> received{0};

    // prefetchCount=2,不 ack,最多投递 2 条
    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        (void)d;
        received++;
        // 不 ack,模拟慢消费者
    }, 2);

    backend.startConsuming();

    // 发布 5 条消息
    for (int i = 0; i < 5; ++i) {
        AmqpMessage msg;
        msg.exchange = "ex";
        msg.routingKey = "key";
        msg.body = QString::number(i).toUtf8();
        (void)backend.publish(msg);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backend.stopConsuming();

    // 受 prefetchCount=2 限制,只应收到 2 条
    QCOMPARE(received.load(), 2);
}

void TestInMemoryAmqpBackend::testMultipleQueuesSingleConsumer() {
    // InMemoryAmqpBackend 当前每队列支持单消费者(m_consumers[queue]= 覆盖语义)
    // 本测试验证:多个队列各自有消费者时,消息能正确路由到对应队列的消费者
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

    // 分别向 q1 和 q2 发送消息
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

    // 两个队列各收到 3 条消息
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

    // 取消订阅
    QVERIFY(backend.cancelConsume("q1").isOk());

    // 取消不存在的订阅
    auto r = backend.cancelConsume("nonexistent");
    QVERIFY(r.isOk());  // erase 不存在的 key 也返回 ok

    backend.stopConsuming();
}

void TestInMemoryAmqpBackend::testCancelConsumeInCallback() {
    // 回归测试: 回调中调用 cancelConsume 不应导致 use-after-free
    // 之前实现持有 ConsumerInfo& 引用跨 callback,回调中 cancel 会失效
    InMemoryAmqpBackend backend;
    (void)backend.connect({});

    (void)backend.declareExchange("ex", ExchangeType::Direct);
    (void)backend.declareQueue("q1");
    (void)backend.bindQueue("q1", "ex", "key");

    std::atomic<int> count{0};
    (void)backend.consume("q1", [&](const AmqpDelivery& d) {
        count++;
        // 在回调中取消自己 — 这会删除 ConsumerInfo
        (void)backend.cancelConsume("q1");
        (void)backend.ack(d.deliveryTag);
    });

    backend.startConsuming();

    // 发布 3 条消息
    for (int i = 0; i < 3; ++i) {
        AmqpMessage msg;
        msg.exchange = "ex";
        msg.routingKey = "key";
        msg.body = QByteArray::number(i);
        (void)backend.publish(msg);
    }

    // 等待分发
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    backend.stopConsuming();

    // 第一条消息触发回调后 cancelConsume,后续消息不应再被投递
    // 验证:无崩溃 + count == 1
    QCOMPARE(count.load(), 1);
}

void TestInMemoryAmqpBackend::testTopicPatternMatching_data() {
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("key");
    QTest::addColumn<bool>("expected");

    // 基本匹配
    QTest::newRow("exact_match") << "logs.error" << "logs.error" << true;
    QTest::newRow("exact_no_match") << "logs.error" << "logs.info" << false;

    // * 通配符
    QTest::newRow("star_match") << "logs.*.error" << "logs.app.error" << true;
    QTest::newRow("star_no_match_extra") << "logs.*.error" << "logs.app.db.error" << false;
    QTest::newRow("star_no_match_less") << "logs.*.error" << "logs.error" << false;

    // # 通配符
    QTest::newRow("hash_match_zero") << "logs.#" << "logs" << true;
    QTest::newRow("hash_match_one") << "logs.#" << "logs.app" << true;
    QTest::newRow("hash_match_many") << "logs.#" << "logs.app.db.critical" << true;
    QTest::newRow("hash_no_match") << "logs.#" << "metrics.app" << false;

    // 组合
    QTest::newRow("combo_match") << "*.app.#" << "logs.app.error.critical" << true;
    QTest::newRow("combo_no_match") << "*.app.#" << "logs.system.error" << false;
}

void TestInMemoryAmqpBackend::testTopicPatternMatching() {
    QFETCH(QString, pattern);
    QFETCH(QString, key);
    QFETCH(bool, expected);

    // matchTopicPattern 是 private static,通过 publish/consume 间接测试
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

/**
 * @class TestRabbitMQConnection
 * @brief RabbitMQConnection 集成测试(使用 InMemory 后端)
 */
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

    // RabbitMQProducer 特有方法(非 IMQProducer 接口),需要 dynamic_pointer_cast
    auto rmqProducer = std::dynamic_pointer_cast<RabbitMQProducer>(producer);
    QVERIFY(rmqProducer != nullptr);

    // 声明 exchange 和队列
    QVERIFY(rmqProducer->declareExchange("test_ex", "direct").isOk());
    QVERIFY(rmqProducer->bindQueue("test_q", "test_ex", "test_key").isOk());

    // 订阅
    std::atomic<bool> received{false};
    QByteArray receivedBody;

    QVERIFY(consumer->subscribe("test_ex", "test_q", [&](const ConsumeMessage& msg) {
        received = true;
        receivedBody = msg.body;
    }).isOk());

    consumer->start();

    // 发送消息
    QVERIFY(producer->send("test_ex", "test_key", "hello world").isOk());

    // 等待接收
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
    QCOMPARE(conn2, nullptr);  // Kafka 暂未实现
}

QTEST_GUILESS_MAIN(TestInMemoryAmqpBackend)

#include "test_mq.moc"
