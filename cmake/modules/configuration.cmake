# configuration.cmake — 配置管理模块 [v2.5.0]
# 依赖: core + utils + logging

add_library(soul_configuration STATIC
    src/soul/configuration/config.cpp
    src/soul/configuration/config_schema.cpp
    src/soul/configuration/etcd_source.cpp
    src/soul/configuration/ini_configuration.cpp
    src/soul/configuration/json_configuration.cpp
    src/soul/configuration/nacos_source.cpp
    src/soul/configuration/remote_config.cpp
)

target_include_directories(soul_configuration PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_configuration PUBLIC soul_core soul_utils soul_logging)