#ifndef SOUL_MQ_RABBITMQ_CONSUMER_H
#define SOUL_MQ_RABBITMQ_CONSUMER_H

#include <QObject>
#include <QMutex>
#include <QThread>
#include <memory>
#include <map>
#include "soul/mq/imq_consumer.h"
#include "soul/core/error.h"

namespace sc {
namespace mq {

class RabbitMQConnection;

class RabbitMQConsumer : public QObject, public IMQConsumer {
    Q_OBJECT
public:
    explicit RabbitMQConsumer(std::weak_ptr<RabbitMQConnection> connection);
    ~RabbitMQConsumer() override;

    Result<void> subscribe(const QString& topic, ConsumeCallback callback) override;
    Result<void> subscribe(const QString& topic, const QString& queueName, ConsumeCallback callback) override;
    Result<void> unsubscribe(const QString& topic) override;
    void start() override;
    void stop() override;

    void setPrefetchCount(int count);
    int prefetchCount() const;

private slots:
    void onMessageReceived();

private:
    std::weak_ptr<RabbitMQConnection> m_connection;
    mutable QMutex m_mutex;
    std::map<QString, ConsumeCallback> m_subscriptions;
    bool m_running = false;
    int m_prefetchCount = 1;

    QString generateQueueName(const QString& topic);
};

} // namespace mq
} // namespace sc

#endif