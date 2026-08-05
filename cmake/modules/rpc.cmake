# rpc.cmake — RPC 框架模块 [v2.5.0]
# 依赖: core + nlohmann_json

add_library(soul_rpc STATIC
    src/soul/rpc/client_proxy.cpp
    src/soul/rpc/http_transport.cpp
    src/soul/rpc/irpc_transport.cpp
    src/soul/rpc/iserializer.cpp
    src/soul/rpc/service_dispatcher.cpp
    src/soul/rpc/service_registry.cpp
)

target_include_directories(soul_rpc PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_rpc PUBLIC soul_core nlohmann_json::nlohmann_json)