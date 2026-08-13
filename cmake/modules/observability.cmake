# observability.cmake — 可观测性模块 [v2.5.0]
# Layer: Infrastructure (I14) — 通用基础设施
# 依赖: core + logging + async + data + network
# 职责: Counter/Gauge/Histogram指标 + Tracer/Span链路追踪 + JsonSink结构化日志
#       PrometheusExporter/OtlpExporter/HealthChecker/InfoEndpoint/ResourcePoolMonitor

add_library(soul_observability STATIC
    src/soul/observability/json_sink.cpp
    src/soul/observability/metrics.cpp
    src/soul/observability/module.cpp
    src/soul/observability/otlp_exporter.cpp
    src/soul/observability/resource_pool_monitor.cpp
    src/soul/observability/tracing.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/observability/otlp_exporter.h
)

target_include_directories(soul_observability PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_observability PUBLIC
    soul_core
    soul_logging
    soul_async
    soul_data
    soul_network
)