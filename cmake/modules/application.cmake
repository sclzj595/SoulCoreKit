# application.cmake — 应用上下文模块 [v3.0.0]
# Layer: Application (A03) — 应用上下文与 CS 架构集成
# 依赖: core + di + configuration + cs (v3.0: ApplicationContext 使用 CsRouter/CsErrorHandler)
# 职责: ApplicationContext/ControllerRegistry/ServiceRegistry

add_library(soul_application STATIC
    src/soul/application/application_context.cpp
    src/soul/application/controller_registry.cpp
    src/soul/application/service_registry.cpp
)

target_include_directories(soul_application PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_application PUBLIC soul_core soul_di soul_configuration soul_cs)