#include "soul/mq/module.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"

namespace sc {
namespace mq {

MQModule::MQModule() : Module("mq") {
}

Result<void> MQModule::init() {
    SC_INFO("Initializing MQ module");
    SC_INFO("MQ module initialized successfully");
    return Result<void>::ok();
}


void MQModule::cleanup() {
    SC_INFO("Cleaning up MQ module");
}

} // namespace mq
} // namespace sc
