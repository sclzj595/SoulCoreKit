#include "soul/mq/rabbitmq/rabbitmq_connection.h"
#include "soul/mq/rabbitmq/rabbitmq_producer.h"
#include "soul/mq/rabbitmq/rabbitmq_consumer.h"
#include "soul/mq/inmemory_amqp_backend.h"
#include "soul/logging/log_macros.h"
#include "soul/core/result.h"
#include <memory>

// 条件包含 AmqpCppBackend(仅 ENABLE_RABBITMQ=ON 时编译)
#ifdef SOUL_ENABLE_RABBITMQ
#include "soul/mq/amqpcpp_backend.h"
#endif

namespace sc {
namespace mq {

RabbitMQConnection::RabbitMQConnection() {
    QObject::connect(&m_heartbeatTimer, &QTimer::timeout, this, &RabbitMQConnection::onHeartbeat);
    QObject::connect(&m_reconnectTimer, &QTimer::timeout, this, &RabbitMQConnection::onReconnectTimer);
}

RabbitMQConnection::~RabbitMQConnection() {
    disconnect();
}

std::shared_ptr<IAmqpBackend> RabbitMQConnection::createBackend(BackendType type) {
    switch (type) {
    case BackendType::InMemory:
        return std::make_shared<InMemoryAmqpBackend>();
    case BackendType::AmqpCpp:
#ifdef SOUL_ENABLE_RABBITMQ
        return std::make_shared<AmqpCppBackend>();
#else
        // 未启用 amqpcpp 编译时,返回 nullptr,由 connect() 返回 NotImplemented
        // 不再静默 fallback 到 InMemory,避免误导用户
        SC_ERROR("RabbitMQConnection: AmqpCpp backend requested but "
                 "ENABLE_RABBITMQ=OFF (rebuild with -DENABLE_RABBITMQ=ON)");
        return nullptr;
#endif
    }
    return nullptr;
}

IAmqpBackend::Config RabbitMQConnection::toBackendConfig(const ConnectionConfig& config) {
    IAmqpBackend::Config bc;
    bc.host = config.host;
    bc.port = config.port;
    bc.username = config.username;
    bc.password = config.password;
    bc.virtualHost = config.virtualHost;
    bc.connectionTimeout = config.connectionTimeout;
    bc.heartbeatInterval = config.heartbeatInterval;
    bc.maxChannels = config.maxChannels;
    // SSL/TLS 配置映射(之前遗漏,导致 AmqpCpp 后端无法使用 TLS)
    bc.enableSsl = config.enableSsl;
    bc.caCertPath = config.caCertPath;
    bc.clientCertPath = config.clientCertPath;
    bc.clientKeyPath = config.clientKeyPath;
    return bc;
}

Result<void> RabbitMQConnection::connect(const ConnectionConfig& config) {
    QMutexLocker lock(&m_mutex);
    m_config = config;
    m_reconnectAttempts = 0;

    // 创建后端(若尚未创建或后端类型变更)
    m_backend = createBackend(m_backendType);
    if (!m_backend) {
        // AmqpCpp 后端未编译时返回 NotImplemented,引导用户启用编译选项
        return Result<void>::err(Error(ErrorCode::NotImplemented,
                                        "RabbitMQConnection: AmqpCpp backend not compiled "
                                        "(rebuild with -DENABLE_RABBITMQ=ON)"));
    }

    SC_INFO("Connecting to RabbitMQ: " + config.host.toStdString() +
            ":" + std::to_string(config.port) +
            " (backend=" + std::string(m_backendType == BackendType::InMemory ? "InMemory" : "AmqpCpp") +
            ")");

    auto result = m_backend->connect(toBackendConfig(config));
    if (!result.isOk()) {
        SC_ERROR("RabbitMQ connection failed: " + result.unwrapErr().message().toStdString());
        return result;
    }

    startHeartbeat();
    SC_INFO("RabbitMQ connection established");
    return Result<void>::ok();
}

void RabbitMQConnection::disconnect() {
    QMutexLocker lock(&m_mutex);
    if (m_backend && m_backend->isConnected()) {
        m_backend->disconnect();
    }
    stopHeartbeat();
    m_reconnectTimer.stop();
    SC_INFO("RabbitMQ connection closed");
}

bool RabbitMQConnection::isConnected() const {
    QMutexLocker lock(&m_mutex);
    return m_backend && m_backend->isConnected();
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
        SC_INFO("Attempting reconnection to RabbitMQ, attempt: " +
                std::to_string(m_reconnectAttempts));
        (void)connect(m_config);
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
