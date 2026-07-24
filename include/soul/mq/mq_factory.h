#ifndef SOUL_MQ_MQ_FACTORY_H
#define SOUL_MQ_MQ_FACTORY_H

#include <memory>
#include "soul/mq/imq_connection.h"
#include "soul/mq/rabbitmq/rabbitmq_connection.h"

namespace sc {
namespace mq {

enum class MQType {
    RabbitMQ,
    Kafka,
    RocketMQ
};

class MQFactory {
public:
    static std::shared_ptr<IMQConnection> createConnection(MQType type) {
        switch (type) {
        case MQType::RabbitMQ:
            return std::make_shared<RabbitMQConnection>();
        default:
            return nullptr;
        }
    }
};

} // namespace mq
} // namespace sc

#endif