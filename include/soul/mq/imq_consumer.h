#ifndef SOUL_MQ_IMQ_CONSUMER_H
#define SOUL_MQ_IMQ_CONSUMER_H

#include <QString>
#include <QByteArray>
#include <functional>
#include "soul/core/interface.h"
#include "soul/core/result.h"

namespace sc {
namespace mq {

struct ConsumeMessage {
    QString topic;
    QString routingKey;
    QByteArray body;
    QString messageId;
    QString correlationId;
    qint64 deliveryTag;
};

using ConsumeCallback = std::function<void(const ConsumeMessage& msg)>;

class IMQConsumer : public IInterface {
public:
    ~IMQConsumer() override = default;

    virtual Result<void> subscribe(const QString& topic, ConsumeCallback callback) = 0;
    virtual Result<void> subscribe(const QString& topic, const QString& queueName, ConsumeCallback callback) = 0;
    virtual Result<void> unsubscribe(const QString& topic) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

    std::string interfaceName() const override { return "IMQConsumer"; }
};

} // namespace mq
} // namespace sc

#endif