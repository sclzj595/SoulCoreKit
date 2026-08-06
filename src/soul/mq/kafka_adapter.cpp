#include "soul/mq/kafka_adapter.h"

namespace sc {
namespace mq {

// ============================================================================
// KafkaConnection
// ============================================================================
KafkaConnection::KafkaConnection() = default;

KafkaConnection::~KafkaConnection() {
    disconnect();
}

Result<void> KafkaConnection::connect(const ConnectionConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_brokerHost = config.host;
    m_brokerPort = config.port;
    m_connected = true;
    return Result<void>::ok();
}

void KafkaConnection::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
}

bool KafkaConnection::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

std::shared_ptr<IMQProducer> KafkaConnection::createProducer() {
    return std::make_shared<KafkaProducer>(std::shared_ptr<KafkaConnection>(
        this, [](KafkaConnection*) {}));
}

std::shared_ptr<IMQConsumer> KafkaConnection::createConsumer() {
    return std::make_shared<KafkaConsumer>(std::shared_ptr<KafkaConnection>(
        this, [](KafkaConnection*) {}));
}

void KafkaConnection::setKafkaConfig(const KafkaConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_kafkaConfig = config;
}

// ============================================================================
// KafkaProducer
// ============================================================================
KafkaProducer::KafkaProducer(std::shared_ptr<KafkaConnection> connection)
    : m_connection(std::move(connection)) {
}

KafkaProducer::~KafkaProducer() = default;

Result<void> KafkaProducer::send(const Message& message) {
    Q_UNUSED(message);
    return Result<void>::ok();
}

void KafkaProducer::sendAsync(const Message& message, SendCallback callback) {
    Q_UNUSED(message);
    if (callback) {
        callback(Result<void>::ok());
    }
}

Result<void> KafkaProducer::send(const QString& topic, const QByteArray& body) {
    Q_UNUSED(topic);
    Q_UNUSED(body);
    return Result<void>::ok();
}

Result<void> KafkaProducer::send(const QString& topic, const QString& routingKey, const QByteArray& body) {
    Q_UNUSED(topic);
    Q_UNUSED(routingKey);
    Q_UNUSED(body);
    return Result<void>::ok();
}

Result<void> KafkaProducer::flush(int timeoutMs) {
    Q_UNUSED(timeoutMs);
    return Result<void>::ok();
}

Result<void> KafkaProducer::initTransactions() {
    return Result<void>::ok();
}

Result<void> KafkaProducer::beginTransaction() {
    return Result<void>::ok();
}

Result<void> KafkaProducer::commitTransaction() {
    return Result<void>::ok();
}

Result<void> KafkaProducer::abortTransaction() {
    return Result<void>::ok();
}

// ============================================================================
// KafkaConsumer
// ============================================================================
KafkaConsumer::KafkaConsumer(std::shared_ptr<KafkaConnection> connection)
    : m_connection(std::move(connection)) {
}

KafkaConsumer::~KafkaConsumer() {
    stop();
}

Result<void> KafkaConsumer::subscribe(const QString& topic, ConsumeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks[topic] = std::move(callback);
    return Result<void>::ok();
}

Result<void> KafkaConsumer::subscribe(const QString& topic, const QString& queueName, ConsumeCallback callback) {
    Q_UNUSED(queueName);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks[topic] = std::move(callback);
    return Result<void>::ok();
}

Result<void> KafkaConsumer::unsubscribe(const QString& topic) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.remove(topic);
    return Result<void>::ok();
}

void KafkaConsumer::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = true;
}

void KafkaConsumer::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
}

Result<void> KafkaConsumer::commitOffset(const QString& topic, int partition, qint64 offset) {
    Q_UNUSED(topic);
    Q_UNUSED(partition);
    Q_UNUSED(offset);
    return Result<void>::ok();
}

Result<void> KafkaConsumer::seek(const QString& topic, int partition, qint64 offset) {
    Q_UNUSED(topic);
    Q_UNUSED(partition);
    Q_UNUSED(offset);
    return Result<void>::ok();
}

Result<void> KafkaConsumer::pause(const QString& topic) {
    Q_UNUSED(topic);
    return Result<void>::ok();
}

Result<void> KafkaConsumer::resume(const QString& topic) {
    Q_UNUSED(topic);
    return Result<void>::ok();
}

// ============================================================================
// RocketMQConnection
// ============================================================================
RocketMQConnection::RocketMQConnection() = default;

RocketMQConnection::~RocketMQConnection() {
    disconnect();
}

Result<void> RocketMQConnection::connect(const ConnectionConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_nameServerHost = config.host;
    m_nameServerPort = config.port;
    m_connected = true;
    return Result<void>::ok();
}

void RocketMQConnection::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
}

bool RocketMQConnection::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

std::shared_ptr<IMQProducer> RocketMQConnection::createProducer() {
    return std::make_shared<RocketMQProducer>(std::shared_ptr<RocketMQConnection>(
        this, [](RocketMQConnection*) {}));
}

std::shared_ptr<IMQConsumer> RocketMQConnection::createConsumer() {
    return std::make_shared<RocketMQConsumer>(std::shared_ptr<RocketMQConnection>(
        this, [](RocketMQConnection*) {}));
}

void RocketMQConnection::setRocketMQConfig(const RocketMQConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_rocketMQConfig = config;
}

// ============================================================================
// RocketMQProducer
// ============================================================================
RocketMQProducer::RocketMQProducer(std::shared_ptr<RocketMQConnection> connection)
    : m_connection(std::move(connection)) {
}

RocketMQProducer::~RocketMQProducer() = default;

Result<void> RocketMQProducer::send(const Message& message) {
    Q_UNUSED(message);
    return Result<void>::ok();
}

void RocketMQProducer::sendAsync(const Message& message, SendCallback callback) {
    Q_UNUSED(message);
    if (callback) {
        callback(Result<void>::ok());
    }
}

Result<void> RocketMQProducer::send(const QString& topic, const QByteArray& body) {
    Q_UNUSED(topic);
    Q_UNUSED(body);
    return Result<void>::ok();
}

Result<void> RocketMQProducer::send(const QString& topic, const QString& routingKey, const QByteArray& body) {
    Q_UNUSED(topic);
    Q_UNUSED(routingKey);
    Q_UNUSED(body);
    return Result<void>::ok();
}

Result<void> RocketMQProducer::sendOneway(const Message& message) {
    Q_UNUSED(message);
    return Result<void>::ok();
}

Result<void> RocketMQProducer::sendOrderly(const Message& message, const QString& shardingKey) {
    Q_UNUSED(message);
    Q_UNUSED(shardingKey);
    return Result<void>::ok();
}

// ============================================================================
// RocketMQConsumer
// ============================================================================
RocketMQConsumer::RocketMQConsumer(std::shared_ptr<RocketMQConnection> connection)
    : m_connection(std::move(connection)) {
}

RocketMQConsumer::~RocketMQConsumer() {
    stop();
}

Result<void> RocketMQConsumer::subscribe(const QString& topic, ConsumeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks[topic] = std::move(callback);
    return Result<void>::ok();
}

Result<void> RocketMQConsumer::subscribe(const QString& topic, const QString& queueName, ConsumeCallback callback) {
    Q_UNUSED(queueName);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks[topic] = std::move(callback);
    return Result<void>::ok();
}

Result<void> RocketMQConsumer::unsubscribe(const QString& topic) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.remove(topic);
    return Result<void>::ok();
}

void RocketMQConsumer::start() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = true;
}

void RocketMQConsumer::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
}

Result<void> RocketMQConsumer::subscribeOrderly(const QString& topic, ConsumeCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks[topic] = std::move(callback);
    return Result<void>::ok();
}

Result<void> RocketMQConsumer::suspend() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
    return Result<void>::ok();
}

Result<void> RocketMQConsumer::resume() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = true;
    return Result<void>::ok();
}

} // namespace mq
} // namespace sc