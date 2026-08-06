#include "soul/observability/module.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"
#include "soul/di/container.h"
#include "soul/observability/otlp_exporter.h"

namespace sc {
namespace observability {

ObsModule::ObsModule() : Module("observability") {
}

Result<void> ObsModule::init() {
    SC_INFO("Initializing Observability module");

    auto& container = di::Container::instance();

    // 注册 OtlpHttpExporter 单例
    auto result = container.bindInstance(&OtlpHttpExporter::instance());
    if (!result.isOk()) {
        SC_WARN("OtlpHttpExporter already registered in DI container");
    }

    SC_INFO("Observability module initialized successfully");
    return Result<void>::ok();
}

void ObsModule::cleanup() {
    SC_INFO("Cleaning up Observability module");
    OtlpHttpExporter::instance().shutdown();
}

} // namespace observability
} // namespace sc