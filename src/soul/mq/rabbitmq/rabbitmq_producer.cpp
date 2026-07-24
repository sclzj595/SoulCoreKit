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

    SC_DEBUG("Sending message to topic: " + message.topic.toStdString());
    return Result<void>::ok();
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
    SC_INFO("Declaring exchange: " + exchangeName.toStdString() + " type: " + exchangeType.toStdString());
    return Result<void>::ok();
}

Result<void> RabbitMQProducer::bindQueue(const QString& queueName, const QString& exchangeName, const QString&) {
    SC_INFO("Binding queue: " + queueName.toStdString() + " to exchange: " + exchangeName.toStdString());
    return Result<void>::ok();
}

QString RabbitMQProducer::generateMessageId() {
    return QString::fromStdString(Uuid::generate());
}

} // namespace mq
} // namespace sc