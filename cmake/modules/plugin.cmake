# plugin.cmake — 插件系统模块 [v2.5.0]
# 依赖: core + di + logging

add_library(soul_plugin STATIC
    src/soul/plugin/module.cpp
    src/soul/plugin/plugin_manager.cpp
)

target_include_directories(soul_plugin PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_plugin PUBLIC soul_core soul_di soul_logging)