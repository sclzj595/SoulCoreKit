#ifndef SOUL_MQ_KAFKA_ADAPTER_H
#define SOUL_MQ_KAFKA_ADAPTER_H

// ============================================================================
// kafka_adapter.h / rocketmq_adapter.h — Kafka/RocketMQ 适配器 [v2.5.0]
// ============================================================================
// 提供 Kafka 和 RocketMQ 的完整适配器实现，基于:
//   - Kafka: 自定义 TCP 协议实现 (Kafka Protocol)
//   - RocketMQ: HTTP/Remoting 协议实现
//
// 设计原则:
//   - 最小依赖: 不引入 librdkafka / rocketmq-client-cpp
//   - 统一接口: 实现 IMQConnection/IMQProducer/IMQConsumer
//   - 与现有 RabbitMQ 适配器一致的 API 风格
// ============================================================================

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QHash>
#include <QTimer>
#include <memory>
#include <mutex>
#include <functional>

#include "soul/mq/imq_connection.h"
#include "soul/mq/imq_producer.h"
#include "soul/mq/imq_consumer.h"
#include "soul/core/result.h"

class QTcpSocket;

namespace sc {
namespace mq {

// ============================================================================
// KafkaConfig — Kafka 配置
// ============================================================================
struct KafkaConfig {
    QStringList brokers = {"localhost:9092"};   // Broker 列表
    QString clientId = "SoulCoreKit";           // 客户端 ID
    QString groupId = "soul-default-group";     // 消费者组 ID
    int sessionTimeoutMs = 10000;               // 会话超时
    int heartbeatIntervalMs = 3000;             // 心跳间隔
    int maxPollRecords = 500;                   // 最大拉取记录数
    int fetchMinBytes = 1;                      // 最小拉取字节
    int fetchMaxBytes = 50 * 1024 * 1024;       // 最大拉取字节 (50MB)
    int maxPartitionFetchBytes = 10 * 1024 * 1024; // 每分区最大拉取字节
    int requestTimeoutMs = 30000;               // 请求超时
    int retryBackoffMs = 100;                   // 重试退避
    int reconnectBackoffMs = 1000;              // 重连退避
    bool enableAutoCommit = true;               // 自动提交 offset
    int autoCommitIntervalMs = 5000;            // 自动提交间隔
    QString autoOffsetReset = "latest";         // offset 重置策略
    bool enableIdempotence = false;             // 幂等生产者
    int maxInFlightRequests = 5;                // 最大在途请求数
    QString compressionType = "none";           // 压缩类型 (none/gzip/snappy/lz4/zstd)
    QHash<QString, QString> properties;         // 自定义属性
};

// ============================================================================
// KafkaConnection — Kafka 连接实现
// ============================================================================
class KafkaConnection : public IMQConnection {
public:
    KafkaConnection();
    ~KafkaConnection() override;

    Result<void> connect(const ConnectionConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    std::shared_ptr<IMQProducer> createProducer() override;
    std::shared_ptr<IMQConsumer> createConsumer() override;

    std::string interfaceName() const override { return "KafkaConnection"; }

    // === Kafka 特有 ===
    void setKafkaConfig(const KafkaConfig& config);
    const KafkaConfig& kafkaConfig() const { return m_kafkaConfig; }

private:
    friend class KafkaProducer;
    friend class KafkaConsumer;

    QString m_brokerHost;
    int m_brokerPort = 9092;
    bool m_connected = false;
    mutable std::mutex m_mutex;
    KafkaConfig m_kafkaConfig;
};

// ============================================================================
// KafkaProducer — Kafka 生产者实现
// ============================================================================
class KafkaProducer : public IMQProducer {
public:
    explicit KafkaProducer(std::shared_ptr<KafkaConnection> connection);
    ~KafkaProducer() override;

    Result<void> send(const Message& message) override;
    void sendAsync(const Message& message, SendCallback callback) override;
    Result<void> send(const QString& topic, const QByteArray& body) override;
    Result<void> send(const QString& topic, const QString& routingKey, const QByteArray& body) override;

    std::string interfaceName() const override { return "KafkaProducer"; }

    // === Kafka 特有 ===
    Result<void> flush(int timeoutMs = 10000);
    Result<void> initTransactions();
    Result<void> beginTransaction();
    Result<void> commitTransaction();
    Result<void> abortTransaction();

private:
    std::shared_ptr<KafkaConnection> m_connection;
    mutable std::mutex m_mutex;
};

// ============================================================================
// KafkaConsumer — Kafka 消费者实现
// ============================================================================
class KafkaConsumer : public IMQConsumer {
public:
    explicit KafkaConsumer(std::shared_ptr<KafkaConnection> connection);
    ~KafkaConsumer() override;

    Result<void> subscribe(const QString& topic, ConsumeCallback callback) override;
    Result<void> subscribe(const QString& topic, const QString& queueName, ConsumeCallback callback) override;
    Result<void> unsubscribe(const QString& topic) override;
    void start() override;
    void stop() override;

    std::string interfaceName() const override { return "KafkaConsumer"; }

    // === Kafka 特有 ===
    Result<void> commitOffset(const QString& topic, int partition, qint64 offset);
    Result<void> seek(const QString& topic, int partition, qint64 offset);
    Result<void> pause(const QString& topic);
    Result<void> resume(const QString& topic);

private:
    std::shared_ptr<KafkaConnection> m_connection;
    QHash<QString, ConsumeCallback> m_callbacks;
    mutable std::mutex m_mutex;
    bool m_running = false;
};

// ============================================================================
// RocketMQConfig — RocketMQ 配置
// ============================================================================
struct RocketMQConfig {
    QStringList nameServers = {"localhost:9876"};  // NameServer 地址列表
    QString groupId = "soul-default-group";         // 消费者组
    QString accessKey;                              // AccessKey (阿里云)
    QString secretKey;                              // SecretKey (阿里云)
    int sendMsgTimeoutMs = 3000;                    // 发送超时
    int maxMessageSize = 4 * 1024 * 1024;           // 最大消息大小
    int retryTimesWhenSendFailed = 2;               // 发送失败重试次数
    int retryTimesWhenSendAsyncFailed = 2;          // 异步发送失败重试次数
    int compressMsgBodyOverHowmuch = 4096;         // 压缩阈值 (字节)
    int retryAnotherBrokerWhenNotStoreOK = 0;       // 是否重试其他 Broker
    int maxReconsumeTimes = 16;                     // 最大重消费次数
    int consumeMessageBatchMaxSize = 1;             // 批量消费最大消息数
    int pullBatchSize = 32;                         // 拉取批量大小
    int pullIntervalMs = 0;                         // 拉取间隔
    QHash<QString, QString> properties;             // 自定义属性
};

// ============================================================================
// RocketMQConnection — RocketMQ 连接实现
// ============================================================================
class RocketMQConnection : public IMQConnection {
public:
    RocketMQConnection();
    ~RocketMQConnection() override;

    Result<void> connect(const ConnectionConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;

    std::shared_ptr<IMQProducer> createProducer() override;
    std::shared_ptr<IMQConsumer> createConsumer() override;

    std::string interfaceName() const override { return "RocketMQConnection"; }

    // === RocketMQ 特有 ===
    void setRocketMQConfig(const RocketMQConfig& config);
    const RocketMQConfig& rocketMQConfig() const { return m_rocketMQConfig; }

private:
    friend class RocketMQProducer;
    friend class RocketMQConsumer;

    QString m_nameServerHost;
    int m_nameServerPort = 9876;
    bool m_connected = false;
    mutable std::mutex m_mutex;
    RocketMQConfig m_rocketMQConfig;
};

// ============================================================================
// RocketMQProducer — RocketMQ 生产者实现
// ============================================================================
class RocketMQProducer : public IMQProducer {
public:
    explicit RocketMQProducer(std::shared_ptr<RocketMQConnection> connection);
    ~RocketMQProducer() override;

    Result<void> send(const Message& message) override;
    void sendAsync(const Message& message, SendCallback callback) override;
    Result<void> send(const QString& topic, const QByteArray& body) override;
    Result<void> send(const QString& topic, const QString& routingKey, const QByteArray& body) override;

    std::string interfaceName() const override { return "RocketMQProducer"; }

    // === RocketMQ 特有 ===
    Result<void> sendOneway(const Message& message);
    Result<void> sendOrderly(const Message& message, const QString& shardingKey);

private:
    std::shared_ptr<RocketMQConnection> m_connection;
    mutable std::mutex m_mutex;
};

// ============================================================================
// RocketMQConsumer — RocketMQ 消费者实现
// ============================================================================
class RocketMQConsumer : public IMQConsumer {
public:
    explicit RocketMQConsumer(std::shared_ptr<RocketMQConnection> connection);
    ~RocketMQConsumer() override;

    Result<void> subscribe(const QString& topic, ConsumeCallback callback) override;
    Result<void> subscribe(const QString& topic, const QString& queueName, ConsumeCallback callback) override;
    Result<void> unsubscribe(const QString& topic) override;
    void start() override;
    void stop() override;

    std::string interfaceName() const override { return "RocketMQConsumer"; }

    // === RocketMQ 特有 ===
    Result<void> subscribeOrderly(const QString& topic, ConsumeCallback callback);
    Result<void> suspend();
    Result<void> resume();

private:
    std::shared_ptr<RocketMQConnection> m_connection;
    QHash<QString, ConsumeCallback> m_callbacks;
    mutable std::mutex m_mutex;
    bool m_running = false;
};

} // namespace mq
} // namespace sc

#endif // SOUL_MQ_KAFKA_ADAPTER_H