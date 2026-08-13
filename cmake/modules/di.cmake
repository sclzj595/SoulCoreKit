# di.cmake — 依赖注入容器模块 [v2.5.0]
# Layer: Core (C02) — CS/BS 共用极稳定核心
# 依赖: core
# 职责: Container/Singleton/Scoped/Transient/Module/bindNamed/setPrimary/createScope

add_library(soul_di STATIC
    src/soul/di/module.cpp
)

target_include_directories(soul_di PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(soul_di PUBLIC SC_DI_STATIC_LIB)
target_link_libraries(soul_di PUBLIC soul_core)