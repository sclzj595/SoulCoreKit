#ifndef SOUL_MQ_RABBITMQ_CONNECTION_H
#define SOUL_MQ_RABBITMQ_CONNECTION_H

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <memory>
#include "soul/mq/imq_connection.h"
#include "soul/mq/imq_producer.h"
#include "soul/mq/imq_consumer.h"

namespace sc {
namespace mq {

class RabbitMQConnection : public QObject, public IMQConnection, public std::enable_shared_from_this<RabbitMQConnection> {
    Q_OBJECT
public:
    RabbitMQConnection();
    ~RabbitMQConnection() override;

    Result<void> connect(const ConnectionConfig& config) override;
    void disconnect() override;
    bool isConnected() const override;
    std::shared_ptr<IMQProducer> createProducer() override;
    std::shared_ptr<IMQConsumer> createConsumer() override;

    void setReconnectEnabled(bool enabled);
    bool isReconnectEnabled() const;

private slots:
    void onHeartbeat();
    void onReconnectTimer();

private:
    bool m_connected = false;
    ConnectionConfig m_config;
    mutable QMutex m_mutex;
    QTimer m_heartbeatTimer;
    QTimer m_reconnectTimer;
    bool m_reconnectEnabled = true;
    int m_reconnectAttempts = 0;

    void startHeartbeat();
    void stopHeartbeat();
    void scheduleReconnect();
};

} // namespace mq
} // namespace sc

#endif