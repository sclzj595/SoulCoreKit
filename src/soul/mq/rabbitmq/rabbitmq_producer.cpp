#include "soul/mq/rabbitmq/rabbitmq_producer.h"
#include "soul/mq/rabbitmq/rabbitmq_connection.h"
#include "soul/logging/log_macros.h"
#include "soul/core/uuid.h"

using namespace sc;

namespace sc {
namespace mq {

RabbitMQProducer::RabbitMQProducer(std::weak_ptr<RabbitMQConnection> connection)
    : m_connection(connection) {
}

RabbitMQProducer::~RabbitMQProducer() {
}

Result<void> RabbitMQProducer::send(const Message& message) {
    QMutexLocker lock(&m_mutex);

    auto conn = m_connection.lock();
    if (!conn || !conn->isConnected()) {
        SC_ERROR("RabbitMQ producer connection not available");
        return Result<void>::err(Error(ErrorCode::NetworkError, "RabbitMQ producer connection not available"));
    }

    auto backend = conn->backend();
    if (!backend) {
        return Result<void>::err(Error(ErrorCode::InternalError, "RabbitMQ backend not available"));
    }

    // 转换 Message 到 AmqpMessage
    AmqpMessage amqpMsg;
    amqpMsg.exchange = message.topic;
    amqpMsg.routingKey = message.routingKey;
    amqpMsg.body = message.body;
    amqpMsg.messageId = message.messageId;
    amqpMsg.correlationId = message.correlationId;
    amqpMsg.deliveryMode = message.deliveryMode;
    amqpMsg.priority = message.priority;

    SC_DEBUG("Sending message to exchange: " + message.topic.toStdString() +
             " routingKey: " + message.routingKey.toStdString());
    return backend->publish(amqpMsg);
}

void RabbitMQProducer::sendAsync(const Message& message, SendCallback callback) {
    auto result = send(message);
    if (callback) {
        callback(result);
    }
}

Result<void> RabbitMQProducer::send(const QString& topic, const QByteArray& body) {
    Message msg;
    msg.topic = topic;
    msg.body = body;
    msg.messageId = generateMessageId();
    return send(msg);
}

Result<void> RabbitMQProducer::send(const QString& topic, const QString& routingKey, const QByteArray& body) {
    Message msg;
    msg.topic = topic;
    msg.routingKey = routingKey;
    msg.body = body;
    msg.messageId = generateMessageId();
    return send(msg);
}

Result<void> RabbitMQProducer::declareExchange(const QString& exchangeName, const QString& exchangeType) {
    auto conn = m_connection.lock();
    if (!conn || !conn->isConnected()) {
        return Result<void>::err(Error(ErrorCode::NetworkError, "RabbitMQ connection not available"));
    }

    auto backend = conn->backend();
    if (!backend) {
        return Result<void>::err(Error(ErrorCode::InternalError, "RabbitMQ backend not available"));
    }

    // 字符串转枚举
    ExchangeType type = ExchangeType::Direct;
    QString t = exchangeType.toLower().trimmed();
    if (t == "direct") {
        type = ExchangeType::Direct;
    } else if (t == "fanout") {
        type = ExchangeType::Fanout;
    } else if (t == "topic") {
        type = ExchangeType::Topic;
    } else if (t == "headers") {
        type = ExchangeType::Headers;
    } else {
        return Result<void>::err(Error(ErrorCode::InvalidArgument,
                                        "Unknown exchange type: " + exchangeType.toStdString()));
    }

    SC_INFO("Declaring exchange: " + exchangeName.toStdString() +
            " type: " + exchangeType.toStdString());
    return backend->declareExchange(exchangeName, type);
}

Result<void> RabbitMQProducer::bindQueue(const QString& queueName,
                                           const QString& exchangeName,
                                           const QString& routingKey) {
    auto conn = m_connection.lock();
    if (!conn || !conn->isConnected()) {
        return Result<void>::err(Error(ErrorCode::NetworkError, "RabbitMQ connection not available"));
    }

    auto backend = conn->backend();
    if (!backend) {
        return Result<void>::err(Error(ErrorCode::InternalError, "RabbitMQ backend not available"));
    }

    // 声明队列(幂等)
    auto qResult = backend->declareQueue(queueName);
    if (!qResult.isOk()) {
        return qResult.unwrapErr();
    }

    SC_INFO("Binding queue: " + queueName.toStdString() +
            " to exchange: " + exchangeName.toStdString() +
            " routingKey: " + routingKey.toStdString());
    return backend->bindQueue(queueName, exchangeName, routingKey);
}

QString RabbitMQProducer::generateMessageId() {
    return QString::fromStdString(Uuid::generate());
}

} // namespace mq
} // namespace sc
