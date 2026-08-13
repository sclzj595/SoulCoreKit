# core.cmake — 核心类型模块 [v3.0.0]
# Layer: Core (C01) — CS/BS 共用极稳定核心
# 依赖: Qt6::Core
# 职责: Result<T>/Error/Singleton/Scaffold/Environment/FeatureFlag/UUID/Version/Lifecycle
# v3.0.0: configuration.cpp 已迁移到 soul_configuration 模块 (IConfigProvider)

add_library(soul_core STATIC
    src/soul/core/application.cpp
    src/soul/core/banner.cpp
    src/soul/core/request_context.cpp   # v2.8.0
    src/soul/core/health.cpp           # v2.8.0
    src/soul/core/environment.cpp
    src/soul/core/feature_flags.cpp
    src/soul/core/json_feature_flag_provider.cpp
    src/soul/core/module_registry.cpp
    src/soul/core/platform.cpp
    src/soul/core/scaffold.cpp
    src/soul/core/startup_logger.cpp
    src/soul/core/time.cpp
    src/soul/core/uuid.cpp
    src/soul/core/version.cpp
    # v3.0.0: Config provider implementations (used by application.cpp)
    src/soul/configuration/iconfig_provider.cpp
    src/soul/configuration/config_providers.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/core/lifecycle.h
)

target_include_directories(soul_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_core PUBLIC Qt6::Core PRIVATE spdlog::spdlog nlohmann_json::nlohmann_json)