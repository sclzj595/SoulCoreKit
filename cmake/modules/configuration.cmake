# configuration.cmake — 配置管理模块 [v2.9.0]
# Layer: Infrastructure (I13) — 通用基础设施
# 依赖: core + utils + logging
# 职责: JSON/INI多源配置 + 环境变量覆盖 + 热加载 + Profile隔离(dev/test/prod) + ConfigSchema验证
#       ConfigCenterClient/EtcdSource/NacosSource/RemoteConfig
#       v2.9.0: + IConfigProvider + ConfigSnapshot + PriorityConfigChain + 5 Providers

add_library(soul_configuration STATIC
    src/soul/configuration/config.cpp
    src/soul/configuration/config_center_client.cpp
    src/soul/configuration/config_schema.cpp
    src/soul/configuration/etcd_source.cpp
    src/soul/configuration/ini_configuration.cpp
    src/soul/configuration/json_configuration.cpp
    src/soul/configuration/module.cpp
    src/soul/configuration/nacos_source.cpp
    src/soul/configuration/remote_config.cpp
    # v3.0.0: Config provider 实现已移至 soul_core (application.cpp 依赖)
    # v3.0.0: Q_OBJECT 头文件必须加入 sources 以便 AUTOMOC 扫描
    include/soul/configuration/config.h
    include/soul/configuration/remote_config.h
    include/soul/configuration/config_center_client.h
)

target_include_directories(soul_configuration PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_configuration PUBLIC soul_core soul_utils soul_logging Qt6::Network)