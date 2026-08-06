# observability.cmake — 可观测性模块 [v2.5.0]
# 依赖: core + logging + async + data + network

add_library(soul_observability STATIC
    src/soul/observability/json_sink.cpp
    src/soul/observability/metrics.cpp
    src/soul/observability/module.cpp
    src/soul/observability/otlp_exporter.cpp
    src/soul/observability/resource_pool_monitor.cpp
    src/soul/observability/tracing.cpp
)

target_include_directories(soul_observability PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_observability PUBLIC
    soul_core
    soul_logging
    soul_async
    soul_data
    soul_network
)