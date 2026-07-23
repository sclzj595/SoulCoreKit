#include "soul/orm/module.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"

namespace sc {
namespace orm {

ORMModule::ORMModule() : Module("orm") {
}

Result<void> ORMModule::init() {
    SC_INFO("Initializing ORM module");
    SC_INFO("ORM module initialized successfully");
    return Result<void>::ok();
}


void ORMModule::cleanup() {
    SC_INFO("Cleaning up ORM module");
}

} // namespace orm
} // namespace sc
