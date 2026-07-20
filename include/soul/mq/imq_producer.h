#ifndef SOUL_MQ_IMQ_PRODUCER_H
#define SOUL_MQ_IMQ_PRODUCER_H

#include <QString>
#include <QByteArray>
#include <functional>
#include "soul/core/interface.h"
#include "soul/core/result.h"

namespace sc {
namespace mq {

struct Message {
    QString topic;
    QString routingKey;
    QByteArray body;
    int priority = 0;
    int deliveryMode = 2;
    QString correlationId;
    QString messageId;
};

using SendCallback = std::function<void(const Result<void>&)>;

class IMQProducer : public IInterface {
public:
    ~IMQProducer() override = default;

    virtual Result<void> send(const Message& message) = 0;
    virtual void sendAsync(const Message& message, SendCallback callback) = 0;
    virtual Result<void> send(const QString& topic, const QByteArray& body) = 0;
    virtual Result<void> send(const QString& topic, const QString& routingKey, const QByteArray& body) = 0;

    std::string interfaceName() const override { return "IMQProducer"; }
};

} // namespace mq
} // namespace sc

#endif