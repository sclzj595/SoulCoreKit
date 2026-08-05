# application.cmake — 应用上下文模块 [v2.5.0]
# 依赖: core + di

add_library(soul_application STATIC
    src/soul/application/application_context.cpp
    src/soul/application/controller_registry.cpp
    src/soul/application/service_registry.cpp
)

target_include_directories(soul_application PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_application PUBLIC soul_core soul_di)