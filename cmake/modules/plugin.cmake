# plugin.cmake — 插件系统模块 [v2.5.0]
# Layer: Extensions (E04) — 企业级扩展
# 依赖: core + di + logging
# 职责: PluginManager/IPlugin/C-ABI边界/DLL动态加载/PluginMetadata版本兼容检查

add_library(soul_plugin STATIC
    src/soul/plugin/module.cpp
    src/soul/plugin/plugin_manager.cpp
)

target_include_directories(soul_plugin PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(soul_plugin PUBLIC SC_PLUGIN_STATIC_LIB)
target_link_libraries(soul_plugin PUBLIC soul_core soul_di soul_logging)