#include "soul/mq/rabbitmq/rabbitmq_consumer.h"
#include "soul/mq/rabbitmq/rabbitmq_connection.h"
#include "soul/logging/log_macros.h"
#include "soul/core/uuid.h"

using namespace sc;

namespace sc {
namespace mq {

RabbitMQConsumer::RabbitMQConsumer(std::weak_ptr<RabbitMQConnection> connection)
    : m_connection(connection) {
}

RabbitMQConsumer::~RabbitMQConsumer() {
    stop();
}


Result<void> RabbitMQConsumer::subscribe(const QString& topic, ConsumeCallback callback) {
    QString queueName = generateQueueName(topic);
    return subscribe(topic, queueName, callback);
}

Result<void> RabbitMQConsumer::subscribe(const QString& topic, const QString& queueName, ConsumeCallback callback) {
    QMutexLocker lock(&m_mutex);

    auto conn = m_connection.lock();
    if (!conn || !conn->isConnected()) {
        SC_ERROR("RabbitMQ consumer connection not available");
        return Result<void>::err(Error(ErrorCode::NetworkError, "RabbitMQ consumer connection not available"));
    }

    m_subscriptions[topic] = callback;
    SC_INFO("Subscribed to topic: " + topic.toStdString() + " queue: " + queueName.toStdString());
    return Result<void>::ok();
}

Result<void> RabbitMQConsumer::unsubscribe(const QString& topic) {
    QMutexLocker lock(&m_mutex);

    auto it = m_subscriptions.find(topic);
    if (it == m_subscriptions.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound, "Subscription not found for topic: " + topic.toStdString()));
    }

    m_subscriptions.erase(it);
    SC_INFO("Unsubscribed from topic: " + topic.toStdString());
    return Result<void>::ok();
}

void RabbitMQConsumer::start() {
    QMutexLocker lock(&m_mutex);
    m_running = true;
    SC_INFO("RabbitMQ consumer started");
}

void RabbitMQConsumer::stop() {
    QMutexLocker lock(&m_mutex);
    m_running = false;
    SC_INFO("RabbitMQ consumer stopped");
}

void RabbitMQConsumer::setPrefetchCount(int count) {
    m_prefetchCount = count;
}

int RabbitMQConsumer::prefetchCount() const {
    return m_prefetchCount;
}

void RabbitMQConsumer::onMessageReceived() {
}

QString RabbitMQConsumer::generateQueueName(const QString& topic) {
    return topic + "_queue_" + QString::fromStdString(Uuid::generate()).left(8);
}

} // namespace mq
} // namespace sc