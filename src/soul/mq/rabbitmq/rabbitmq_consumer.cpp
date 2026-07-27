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

Result<void> RabbitMQConsumer::subscribe(const QString& topic, const QString& queueName,
                                           ConsumeCallback callback) {
    QMutexLocker lock(&m_mutex);

    auto conn = m_connection.lock();
    if (!conn || !conn->isConnected()) {
        SC_ERROR("RabbitMQ consumer connection not available");
        return Result<void>::err(Error(ErrorCode::NetworkError, "RabbitMQ consumer connection not available"));
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

    // 绑定队列到 exchange
    // 语义: topic 作为 routingKey(订阅主题 = 路由键)
    // - Direct exchange: producer 必须用 routingKey=topic 发送
    // - Fanout exchange: routingKey 被忽略,广播到所有绑定队列
    // - Topic exchange: topic 支持 * 和 # 通配符
    // 若需自定义 routingKey,请先用 RabbitMQProducer::bindQueue 显式绑定
    auto bResult = backend->bindQueue(queueName, topic, topic);
    if (!bResult.isOk()) {
        return bResult.unwrapErr();
    }

    // 包装回调:将 AmqpDelivery 转换为 ConsumeMessage
    AmqpConsumeCallback amqpCallback = [callback](const AmqpDelivery& delivery) {
        ConsumeMessage msg;
        msg.topic = delivery.exchange;
        msg.routingKey = delivery.routingKey;
        msg.body = delivery.message.body;
        msg.messageId = delivery.message.messageId;
        msg.correlationId = delivery.message.correlationId;
        msg.deliveryTag = delivery.deliveryTag;
        callback(msg);
    };

    auto cResult = backend->consume(queueName, amqpCallback, m_prefetchCount);
    if (!cResult.isOk()) {
        return cResult.unwrapErr();
    }

    m_subscriptions[topic] = queueName;
    SC_INFO("Subscribed to topic: " + topic.toStdString() +
            " queue: " + queueName.toStdString());
    return Result<void>::ok();
}

Result<void> RabbitMQConsumer::unsubscribe(const QString& topic) {
    QMutexLocker lock(&m_mutex);

    auto it = m_subscriptions.find(topic);
    if (it == m_subscriptions.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
                                        "Subscription not found for topic: " + topic.toStdString()));
    }

    QString queueName = it->second;

    auto conn = m_connection.lock();
    if (conn && conn->isConnected()) {
        auto backend = conn->backend();
        if (backend) {
            (void)backend->cancelConsume(queueName);
        }
    }

    m_subscriptions.erase(it);
    SC_INFO("Unsubscribed from topic: " + topic.toStdString());
    return Result<void>::ok();
}

void RabbitMQConsumer::start() {
    QMutexLocker lock(&m_mutex);

    auto conn = m_connection.lock();
    if (!conn || !conn->isConnected()) {
        SC_ERROR("RabbitMQ consumer cannot start: connection not available");
        return;
    }

    auto backend = conn->backend();
    if (!backend) {
        SC_ERROR("RabbitMQ consumer cannot start: backend not available");
        return;
    }

    m_running = true;
    backend->startConsuming();
    SC_INFO("RabbitMQ consumer started");
}

void RabbitMQConsumer::stop() {
    QMutexLocker lock(&m_mutex);
    if (!m_running) {
        return;
    }

    m_running = false;

    auto conn = m_connection.lock();
    // 先解锁再调用 backend->stopConsuming(),避免 stopConsuming() 内部 join
    // 分发线程时,分发线程回调中若访问 consumer 加锁方法形成 AB-BA 死锁。
    lock.unlock();

    if (conn && conn->isConnected()) {
        auto backend = conn->backend();
        if (backend) {
            backend->stopConsuming();
        }
    }

    SC_INFO("RabbitMQ consumer stopped");
}

void RabbitMQConsumer::setPrefetchCount(int count) {
    QMutexLocker lock(&m_mutex);
    m_prefetchCount = count;
}

int RabbitMQConsumer::prefetchCount() const {
    QMutexLocker lock(&m_mutex);
    return m_prefetchCount;
}

void RabbitMQConsumer::onMessageReceived() {
    // 由 backend 的 dispatchLoop 直接调用 callback,此方法保留为兼容
}

QString RabbitMQConsumer::generateQueueName(const QString& topic) {
    return topic + "_queue_" + QString::fromStdString(Uuid::generate()).left(8);
}

} // namespace mq
} // namespace sc
