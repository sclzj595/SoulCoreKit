# network_core.cmake — 网络核心抽象层 [v2.5.0]
# Layer: Infrastructure (I01) — 通信基础设施
# 依赖: core + logging
# 职责: NetworkAdapterBase/CodecFactory/JsonCodec/Metrics/Monitor

add_library(soul_network_core STATIC
    src/soul/network/core/network_adapter_base.cpp
    src/soul/network/codec/codec_factory.cpp
    src/soul/network/codec/json_codec.cpp
    src/soul/network/monitor/metrics.cpp
    src/soul/network/monitor/monitor.cpp
    # v3.0.0: network_error.cpp 移至 core 层，避免 soul_network_protocol 循环依赖
    # NetworkError 是 network_core/protocol/http 所有子模块的公共类型
    src/soul/network/network_error.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/network/core/network_base.h
    include/soul/network/core/network_adapter_base.h
    include/soul/network/monitor/monitor.h
)

target_include_directories(soul_network_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(soul_network_core PUBLIC SC_NETWORK_STATIC_LIB)
target_link_libraries(soul_network_core PUBLIC soul_core soul_logging nlohmann_json::nlohmann_json)