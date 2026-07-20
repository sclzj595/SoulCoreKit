#ifndef SOUL_MQ_RABBITMQ_PRODUCER_H
#define SOUL_MQ_RABBITMQ_PRODUCER_H

#include <QObject>
#include <QMutex>
#include <memory>
#include "soul/mq/imq_producer.h"
#include "soul/core/error.h"

namespace sc {
namespace mq {

class RabbitMQConnection;

class RabbitMQProducer : public QObject, public IMQProducer {
    Q_OBJECT
public:
    explicit RabbitMQProducer(std::weak_ptr<RabbitMQConnection> connection);
    ~RabbitMQProducer() override;

    Result<void> send(const Message& message) override;
    void sendAsync(const Message& message, SendCallback callback) override;
    Result<void> send(const QString& topic, const QByteArray& body) override;
    Result<void> send(const QString& topic, const QString& routingKey, const QByteArray& body) override;

    Result<void> declareExchange(const QString& exchangeName, const QString& exchangeType = "direct");
    Result<void> bindQueue(const QString& queueName, const QString& exchangeName, const QString& routingKey);

private:
    std::weak_ptr<RabbitMQConnection> m_connection;
    mutable QMutex m_mutex;

    QString generateMessageId();
};

} // namespace mq
} // namespace sc

#endif