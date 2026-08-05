# network_core.cmake — 网络核心抽象层 [v2.5.0]
# 依赖: core + logging

add_library(soul_network_core STATIC
    src/soul/network/core/network_adapter_base.cpp
    src/soul/network/codec/codec_factory.cpp
    src/soul/network/codec/json_codec.cpp
    src/soul/network/monitor/metrics.cpp
    src/soul/network/monitor/monitor.cpp
)

target_include_directories(soul_network_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_network_core PUBLIC soul_core soul_logging)