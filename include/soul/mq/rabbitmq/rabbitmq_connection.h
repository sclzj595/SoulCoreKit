#ifndef SOUL_MQ_RABBITMQ_CONNECTION_H
#define SOUL_MQ_RABBITMQ_CONNECTION_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <memory>
#include "soul/mq/imq_connection.h"
#include "soul/mq/imq_producer.h"
#include "soul/mq/imq_consumer.h"
#include "soul/mq/iamqp_backend.h"

namespace sc {
namespace mq {

class RabbitMQProducer;
class RabbitMQConsumer;

/**
 * @class RabbitMQConnection
 * @brief RabbitMQ 连接(基于 IAmqpBackend 后端抽象)
 *
 * RabbitMQConnection 通过委托 IAmqpBackend 实现真实的 AMQP 通信。
 * 后端可在 InMemoryAmqpBackend(开发/测试)和 AmqpCppBackend(生产)之间切换,
 * 公共接口 IMQConnection 保持不变。
 *
 * @since v1.7.0 重构为 backend 委托模式
 */
class RabbitMQConnection : public QObject,
                           public IMQConnection,
                           public std::enable_shared_from_this<RabbitMQConnection> {
    Q_OBJECT
public:
    /// @brief 后端类型选择
    enum class BackendType {
        InMemory,  ///< 内存后端(默认,用于开发/测试)
        AmqpCpp    ///< amqpcpp 真实后端(需要 ENABLE_RABBITMQ=ON 编译)
    };

    RabbitMQConnection();
    ~RabbitMQConnection() override;

    // IMQConnection 接口实现
    Result<void> connect(const ConnectionConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::shared_ptr<IMQProducer> createProducer() override;
    std::shared_ptr<IMQConsumer> createConsumer() override;

    /// @brief 设置后端类型(必须在 connect 前调用)
    void setBackendType(BackendType type) { m_backendType = type; }

    /// @brief 获取后端类型
    [[nodiscard]] BackendType backendType() const { return m_backendType; }

    /// @brief 获取底层 backend(供 Producer/Consumer 使用)
    /// @return backend 引用;未连接时返回 nullptr
    std::shared_ptr<IAmqpBackend> backend() {
        QMutexLocker lock(&m_mutex);
        return m_backend;
    }

    void setReconnectEnabled(bool enabled);
    bool isReconnectEnabled() const;

private slots:
    void onHeartbeat();
    void onReconnectTimer();

private:
    BackendType m_backendType = BackendType::InMemory;
    std::shared_ptr<IAmqpBackend> m_backend;
    ConnectionConfig m_config;
    mutable QMutex m_mutex;
    QTimer m_heartbeatTimer;
    QTimer m_reconnectTimer;
    bool m_reconnectEnabled = true;
    int m_reconnectAttempts = 0;

    /// @brief 创建后端实例
    std::shared_ptr<IAmqpBackend> createBackend(BackendType type);

    /// @brief 将 ConnectionConfig 转换为 IAmqpBackend::Config
    static IAmqpBackend::Config toBackendConfig(const ConnectionConfig& config);

    void startHeartbeat();
    void stopHeartbeat();
    void scheduleReconnect();
};

} // namespace mq
} // namespace sc

#endif // SOUL_MQ_RABBITMQ_CONNECTION_H
