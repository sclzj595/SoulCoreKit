#ifndef SOUL_MQ_IMQ_CONNECTION_H
#define SOUL_MQ_IMQ_CONNECTION_H

#include <QString>
#include <memory>
#include "soul/core/interface.h"
#include "soul/core/result.h"

namespace sc {
namespace mq {

class IMQProducer;
class IMQConsumer;

struct ConnectionConfig {
    QString host = "localhost";
    int port = 5672;
    QString username = "guest";
    QString password = "guest";
    QString virtualHost = "/";
    int connectionTimeout = 30000;
    int heartbeatInterval = 60;
    int maxChannels = 10;
};

class IMQConnection : public IInterface {
public:
    ~IMQConnection() override = default;

    virtual Result<void> connect(const ConnectionConfig& config) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual std::shared_ptr<IMQProducer> createProducer() = 0;
    virtual std::shared_ptr<IMQConsumer> createConsumer() = 0;

    std::string interfaceName() const override { return "IMQConnection"; }
};

} // namespace mq
} // namespace sc

#endif