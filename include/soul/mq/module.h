#ifndef SOUL_MQ_MODULE_H
#define SOUL_MQ_MODULE_H

#include "soul/core/module.h"

namespace sc {
namespace mq {

class MQModule : public Module {
public:
    MQModule();
    Result<void> init() override;
    void cleanup() override;
};

} // namespace mq
} // namespace sc

#endif