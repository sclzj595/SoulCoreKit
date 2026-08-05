# core.cmake — 核心类型模块 [v2.5.0]
# 依赖: Qt6::Core

add_library(soul_core STATIC
    src/soul/core/application.cpp
    src/soul/core/banner.cpp
    src/soul/core/configuration.cpp
    src/soul/core/environment.cpp
    src/soul/core/module_registry.cpp
    src/soul/core/platform.cpp
    src/soul/core/scaffold.cpp
    src/soul/core/startup_logger.cpp
    src/soul/core/time.cpp
    src/soul/core/uuid.cpp
    src/soul/core/version.cpp
)

target_include_directories(soul_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_core PUBLIC Qt6::Core)