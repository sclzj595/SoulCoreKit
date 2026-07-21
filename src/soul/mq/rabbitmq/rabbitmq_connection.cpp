#include "soul/mq/rabbitmq/rabbitmq_connection.h"
#include "soul/mq/rabbitmq/rabbitmq_producer.h"
#include "soul/mq/rabbitmq/rabbitmq_consumer.h"
#include "soul/logging/log_macros.h"

namespace sc {
namespace mq {

RabbitMQConnection::RabbitMQConnection() {
    QObject::connect(&m_heartbeatTimer, &QTimer::timeout, this, &RabbitMQConnection::onHeartbeat);
    QObject::connect(&m_reconnectTimer, &QTimer::timeout, this, &RabbitMQConnection::onReconnectTimer);
}

RabbitMQConnection::~RabbitMQConnection() {
    disconnect();
}


Result<void> RabbitMQConnection::connect(const ConnectionConfig& config) {
    QMutexLocker lock(&m_mutex);
    m_config = config;
    m_reconnectAttempts = 0;

    SC_INFO("Connecting to RabbitMQ: " + config.host.toStdString() + ":" + std::to_string(config.port));

    m_connected = true;
    startHeartbeat();

    SC_INFO("RabbitMQ connection established");
    return {};
}

void RabbitMQConnection::disconnect() {
    QMutexLocker lock(&m_mutex);
    if (!m_connected) {
        return;
    }

    stopHeartbeat();
    m_reconnectTimer.stop();
    m_connected = false;

    SC_INFO("RabbitMQ connection closed");
}

bool RabbitMQConnection::isConnected() const {
    QMutexLocker lock(&m_mutex);
    return m_connected;
}

std::shared_ptr<IMQProducer> RabbitMQConnection::createProducer() {
    return std::make_shared<RabbitMQProducer>(shared_from_this());
}

std::shared_ptr<IMQConsumer> RabbitMQConnection::createConsumer() {
    return std::make_shared<RabbitMQConsumer>(shared_from_this());
}

void RabbitMQConnection::setReconnectEnabled(bool enabled) {
    m_reconnectEnabled = enabled;
}

bool RabbitMQConnection::isReconnectEnabled() const {
    return m_reconnectEnabled;
}

void RabbitMQConnection::onHeartbeat() {
    if (!isConnected()) {
        scheduleReconnect();
    }
}

void RabbitMQConnection::onReconnectTimer() {
    m_reconnectTimer.stop();
    if (m_reconnectEnabled) {
        SC_INFO("Attempting reconnection to RabbitMQ, attempt: " + std::to_string(m_reconnectAttempts));
        connect(m_config);
    }
}

void RabbitMQConnection::startHeartbeat() {
    m_heartbeatTimer.start(m_config.heartbeatInterval * 1000);
}

void RabbitMQConnection::stopHeartbeat() {
    m_heartbeatTimer.stop();
}

void RabbitMQConnection::scheduleReconnect() {
    if (!m_reconnectEnabled) {
        return;
    }

    m_reconnectAttempts++;
    int delay = std::min(30000, 1000 * (1 << std::min(m_reconnectAttempts, 10)));
    m_reconnectTimer.start(delay);
}

} // namespace mq
} // namespace sc