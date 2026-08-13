# rpc.cmake — RPC 框架模块 [v2.5.0]
# Layer: Extensions (E01) — 企业级扩展
# 依赖: core + logging + Qt::Network + nlohmann_json
# 职责: RpcClient/RpcServer/ServiceDispatcher/ClientProxy/ServiceDiscovery/ServiceRegistry
#       Transport 抽象: HTTP/WebSocket/TCP 皆可为 Transport，gRPC 作为 Adapter
#       LoadBalancer/HttpTransport/JsonSerializer

add_library(soul_rpc STATIC
    src/soul/rpc/client_proxy.cpp
    src/soul/rpc/grpc_server.cpp
    src/soul/rpc/http_transport.cpp
    src/soul/rpc/irpc_transport.cpp
    src/soul/rpc/iserializer.cpp
    src/soul/rpc/module.cpp
    src/soul/rpc/service_discovery.cpp
    src/soul/rpc/service_dispatcher.cpp
    src/soul/rpc/service_registry.cpp
    # v3.0.0: Q_OBJECT 头文件必须加入 sources 以便 AUTOMOC 扫描
    include/soul/rpc/service_discovery.h
    include/soul/rpc/http_transport.h
    include/soul/rpc/grpc_server.h
)

target_include_directories(soul_rpc PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_rpc PUBLIC soul_core soul_logging Qt6::Network nlohmann_json::nlohmann_json)