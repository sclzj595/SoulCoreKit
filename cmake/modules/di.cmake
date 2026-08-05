# di.cmake — 依赖注入容器模块 [v2.5.0]
# 依赖: core

add_library(soul_di STATIC
    src/soul/di/module.cpp
)

target_include_directories(soul_di PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_di PUBLIC soul_core)