#include <QTest>
#include <QCoreApplication>
#include <QByteArray>
#include "soul/mq/imq_producer.h"
#include "soul/mq/imq_consumer.h"
#include "soul/mq/imq_connection.h"
#include "soul/mq/mq_factory.h"
#include "soul/core/result.h"

using namespace sc::mq;

class TestMQMessage : public QObject {
    Q_OBJECT

private slots:
    void testMessageCreation();
    void testMessageFields();
    void testConsumeMessageCreation();
    void testConsumeMessageFields();
};

void TestMQMessage::testMessageCreation() {
    Message msg;
    QVERIFY(msg.topic.isEmpty());
    QVERIFY(msg.routingKey.isEmpty());
    QVERIFY(msg.body.isEmpty());
    QCOMPARE(msg.priority, 0);
    QCOMPARE(msg.deliveryMode, 2);
}

void TestMQMessage::testMessageFields() {
    Message msg;
    msg.topic = "test.topic";
    msg.routingKey = "routing.key";
    msg.body = "test body";
    msg.priority = 5;
    msg.deliveryMode = 1;
    msg.correlationId = "corr-123";
    msg.messageId = "msg-456";
    
    QCOMPARE(msg.topic, QString("test.topic"));
    QCOMPARE(msg.routingKey, QString("routing.key"));
    QCOMPARE(msg.body, QByteArray("test body"));
    QCOMPARE(msg.priority, 5);
    QCOMPARE(msg.deliveryMode, 1);
    QCOMPARE(msg.correlationId, QString("corr-123"));
    QCOMPARE(msg.messageId, QString("msg-456"));
}

void TestMQMessage::testConsumeMessageCreation() {
    ConsumeMessage msg;
    QVERIFY(msg.topic.isEmpty());
    QVERIFY(msg.routingKey.isEmpty());
    QVERIFY(msg.body.isEmpty());
    QVERIFY(msg.messageId.isEmpty());
    QCOMPARE(msg.deliveryTag, qint64(0));
}

void TestMQMessage::testConsumeMessageFields() {
    ConsumeMessage msg;
    msg.topic = "consume.topic";
    msg.routingKey = "key";
    msg.body = "consumed body";
    msg.messageId = "msg-789";
    msg.correlationId = "corr-789";
    msg.deliveryTag = 12345;
    
    QCOMPARE(msg.topic, QString("consume.topic"));
    QCOMPARE(msg.body, QByteArray("consumed body"));
    QCOMPARE(msg.messageId, QString("msg-789"));
    QCOMPARE(msg.deliveryTag, qint64(12345));
}

class TestMQFactory : public QObject {
    Q_OBJECT

private slots:
    void testCreateRabbitMQConnection();
    void testCreateUnsupportedConnection();
};

void TestMQFactory::testCreateRabbitMQConnection() {
    auto conn = MQFactory::createConnection(MQType::RabbitMQ);
    QVERIFY(conn != nullptr);
}

void TestMQFactory::testCreateUnsupportedConnection() {
    auto conn = MQFactory::createConnection(MQType::Kafka);
    QVERIFY(conn == nullptr);
    
    conn = MQFactory::createConnection(MQType::RocketMQ);
    QVERIFY(conn == nullptr);
}

class TestMQInterface : public QObject {
    Q_OBJECT

private slots:
    void testProducerInterfaceName();
    void testConsumerInterfaceName();
};

void TestMQInterface::testProducerInterfaceName() {
    auto conn = MQFactory::createConnection(MQType::RabbitMQ);
    QVERIFY(conn != nullptr);
    auto producer = conn->createProducer();
    QVERIFY(producer != nullptr);
    QCOMPARE(producer->interfaceName(), std::string("IMQProducer"));
}

void TestMQInterface::testConsumerInterfaceName() {
    auto conn = MQFactory::createConnection(MQType::RabbitMQ);
    QVERIFY(conn != nullptr);
    auto consumer = conn->createConsumer();
    QVERIFY(consumer != nullptr);
    QCOMPARE(consumer->interfaceName(), std::string("IMQConsumer"));
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    int result = 0;

    {
        TestMQMessage t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestMQFactory t;
        result |= QTest::qExec(&t, argc, argv);
    }
    {
        TestMQInterface t;
        result |= QTest::qExec(&t, argc, argv);
    }

    return result;
}

#include "test_mq.moc"