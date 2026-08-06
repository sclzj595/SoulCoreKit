#include "soul/configuration/module.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"
#include "soul/di/container.h"
#include "soul/configuration/config_center_client.h"

namespace sc {

ConfigModule::ConfigModule() : Module("configuration") {
}

Result<void> ConfigModule::init() {
    SC_INFO("Initializing Configuration module");

    auto& container = di::Container::instance();

    // 注册 ConfigCenterClient 单例
    auto result = container.bindInstance(&ConfigCenterClient::instance());
    if (!result.isOk()) {
        SC_WARN("ConfigCenterClient already registered in DI container");
    }

    SC_INFO("Configuration module initialized successfully");
    return Result<void>::ok();
}

void ConfigModule::cleanup() {
    SC_INFO("Cleaning up Configuration module");
    ConfigCenterClient::instance().shutdown();
}

} // namespace sc