#include "soul/rpc/module.h"
#include "soul/logging/log_macros.h"
#include "soul/core/error.h"
#include "soul/di/container.h"
#include "soul/rpc/grpc_server.h"
#include "soul/rpc/service_discovery.h"

namespace sc {
namespace rpc {

RpcModule::RpcModule() : Module("rpc") {
}

Result<void> RpcModule::init() {
    SC_INFO("Initializing RPC module");

    auto& container = di::Container::instance();

    // 注册 GrpcServer 单例
    auto result = container.bindInstance(&GrpcServer::instance());
    if (!result.isOk()) {
        SC_WARN("GrpcServer already registered in DI container");
    }

    // 注册 GrpcClient (非单例，使用 bindSingleton 延迟创建)
    result = container.bindSingleton<GrpcClient>([]() {
        return new GrpcClient("localhost:50051");
    });
    if (!result.isOk()) {
        SC_WARN("GrpcClient already registered in DI container");
    }

    // 注册 ServiceDiscoveryFactory
    result = container.bindSingleton<ServiceDiscoveryFactory>([]() {
        return new ServiceDiscoveryFactory();
    });
    if (!result.isOk()) {
        SC_WARN("ServiceDiscoveryFactory already registered in DI container");
    }

    SC_INFO("RPC module initialized successfully");
    return Result<void>::ok();
}

void RpcModule::cleanup() {
    SC_INFO("Cleaning up RPC module");
    GrpcServer::instance().stop();
}

} // namespace rpc
} // namespace sc