cmake_minimum_required(VERSION 3.16)
project(SoulCoreKit
    VERSION 3.0.0
    DESCRIPTION "面向 CS + BS 场景的 C++/Qt 通用应用基础平台"
    LANGUAGES CXX
)

include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(WIN32)
    add_definitions(-DNOMINMAX)
endif()

option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_DOCS "Build documentation" OFF)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(ENABLE_WARNINGS "Enable compiler warnings" ON)
option(ENABLE_SANITIZERS "Enable sanitizers (ASAN/UBSAN)" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer (TSan)" OFF)
option(ENABLE_LTO "Enable Link-Time Optimization" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_IPO "Enable Inter-Procedural Optimization" OFF)
option(ENABLE_CXX20 "Enable C++20 coroutine support" OFF)

set(QT6_DIR "" CACHE PATH "Qt6 CMake config directory (optional)")
if(QT6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${QT6_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Sql Xml)

# AUTOMOC 必须在 find_package(Qt6) 之后设置以确保 Qt6 的 CMake 模块不会覆盖
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOMOC_MOC_OPTIONS "-I${CMAKE_SOURCE_DIR}/include")

# ============================================================================
# nlohmann/json - 全局 JSON 处理(v1.9.2 统一 JSON 方案)
# ============================================================================
# 用于配置、RPC、认证、存储等所有非 UI 模块的 JSON 序列化。
# Qt JSON (QJsonDocument/QJsonObject) 仅保留用于 UI 临时数据序列化。
# nlohmann/json 是 header-only 库,三级备选策略:
#   1. find_package(nlohmann_json) — CI 环境通过 apt install nlohmann-json3-dev
#   2. 本地预装路径 G:/MinGW/nlohmann — 开发者本地 Windows 环境
#   3. FetchContent Git shallow clone — 最终备选,需网络访问 GitHub
find_package(nlohmann_json 3.10.0 QUIET)
if(nlohmann_json_FOUND)
    message(STATUS "Using nlohmann/json v${nlohmann_json_VERSION} via find_package")
elseif(EXISTS "G:/MinGW/nlohmann/json.hpp")
    message(STATUS "Using local nlohmann/json from G:/MinGW/nlohmann")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE "G:/MinGW")
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    install(TARGETS nlohmann_json EXPORT SoulCoreKitTargets)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/build_closure/_deps/nlohmann_json-src/single_include/nlohmann/json.hpp")
    message(STATUS "Using cached nlohmann/json from build_closure/_deps/nlohmann_json-src")
    add_library(nlohmann_json INTERFACE)
    target_include_directories(nlohmann_json INTERFACE "${CMAKE_SOURCE_DIR}/build_closure/_deps/nlohmann_json-src/single_include")
    add_library(nlohmann_json::nlohmann_json ALIAS nlohmann_json)
    install(TARGETS nlohmann_json EXPORT SoulCoreKitTargets)
else()
    message(STATUS "Fetching nlohmann/json v3.11.3 via FetchContent (shallow clone)...")
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(nlohmann_json)
    message(STATUS "nlohmann/json v3.11.3 fetched via FetchContent successfully")
endif()

# ============================================================================
# spdlog - 高性能结构化日志库 (v1.9.3 集成)
# ============================================================================
# 替代自研 Logger,提供 fmt 风格格式化 / 异步日志 / JSON 输出
# 三级备选策略:
#   1. find_package(spdlog) — CI 环境通过 apt install libspdlog-dev
#   2. 本地预装路径 G:/MinGW/spdlog — 开发者本地 Windows 环境
#   3. FetchContent Git shallow clone — 最终备选,需网络访问 GitHub
find_package(spdlog 1.12.0 QUIET)
if(spdlog_FOUND)
    message(STATUS "Using spdlog v${spdlog_VERSION} via find_package")
elseif(EXISTS "G:/MinGW/spdlog/include/spdlog/spdlog.h")
    message(STATUS "Using local spdlog from G:/MinGW/spdlog")
    add_library(spdlog INTERFACE)
    target_include_directories(spdlog INTERFACE "G:/MinGW/spdlog/include")
    add_library(spdlog::spdlog ALIAS spdlog)
    install(TARGETS spdlog EXPORT SoulCoreKitTargets)
elseif(EXISTS "${CMAKE_SOURCE_DIR}/build_closure/_deps/spdlog-src/include/spdlog/spdlog.h")
    message(STATUS "Using cached spdlog from build_closure/_deps/spdlog-src")
    add_subdirectory("${CMAKE_SOURCE_DIR}/build_closure/_deps/spdlog-src" "${CMAKE_SOURCE_DIR}/build_closure/_deps/spdlog-build")
    install(TARGETS spdlog EXPORT SoulCoreKitTargets)
else()
    message(STATUS "Fetching spdlog v1.14.1 via FetchContent (shallow clone)...")
    include(FetchContent)
    FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG        v1.14.1
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   TRUE
    )
    set(SPDLOG_BUILD_EXAMPLE OFF CACHE INTERNAL "")
    set(SPDLOG_BUILD_TESTS OFF CACHE INTERNAL "")
    set(SPDLOG_FMT_EXTERNAL OFF CACHE INTERNAL "")
    FetchContent_MakeAvailable(spdlog)
    install(TARGETS spdlog EXPORT SoulCoreKitTargets)
    message(STATUS "spdlog v1.14.1 fetched via FetchContent successfully")
endif()

# ZLIB: compress_utils.cpp uses deflate/inflate directly.
# On Linux we include system <zlib.h>; on Windows/macOS we use Qt's <QtZlib/zlib.h>.
# find_package(ZLIB) is REQUIRED on Linux (system zlib), but QUIET on Win/macOS
# (where Qt provides zlib symbols transitively via Qt6::Core).
if(UNIX AND NOT APPLE)
    find_package(ZLIB REQUIRED)
else()
    find_package(ZLIB QUIET)
endif()

# 尝试加载 Gui 和 Widgets（可能因为 GuiTools/WidgetsTools 缺失而失败）
# 在某些 Qt 安装中（如仅安装了核心模块），Gui 的工具包可能缺失
# 此时我们手动添加头文件路径作为后备方案
find_package(Qt6 COMPONENTS Widgets Gui QUIET)

if(NOT TARGET Qt6::Gui)
    # 后备方案：手动添加 QtGui 头文件路径和库
    set(QT_GUI_INCLUDE_DIR "${Qt6_DIR}/../../../include/QtGui")
    set(QT_GUI_LIB_DIR "${Qt6_DIR}/../../../lib")
    if(EXISTS "${QT_GUI_INCLUDE_DIR}")
        message(STATUS "Using fallback QtGui include dir: ${QT_GUI_INCLUDE_DIR}")
        add_library(Qt6::Gui INTERFACE IMPORTED)
        set_target_properties(Qt6::Gui PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${QT_GUI_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${QT_GUI_LIB_DIR}/libQt6Gui.a"
        )
    endif()
endif()

if(NOT TARGET Qt6::Widgets)
    set(QT_WIDGETS_INCLUDE_DIR "${Qt6_DIR}/../../../include/QtWidgets")
    set(QT_WIDGETS_LIB_DIR "${Qt6_DIR}/../../../lib")
    if(EXISTS "${QT_WIDGETS_INCLUDE_DIR}")
        message(STATUS "Using fallback QtWidgets include dir: ${QT_WIDGETS_INCLUDE_DIR}")
        add_library(Qt6::Widgets INTERFACE IMPORTED)
        set_target_properties(Qt6::Widgets PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${QT_WIDGETS_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${QT_WIDGETS_LIB_DIR}/libQt6Widgets.a"
        )
    endif()
endif()

if(ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options(/W4 /WX /permissive-)
    else()
        add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wshadow -Wnon-virtual-dtor -Wold-style-cast)
    endif()
endif()

if(ENABLE_SANITIZERS)
    if(MSVC)
        add_compile_options(/fsanitize=address /fsanitize=undefined)
    else()
        add_compile_options(-fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()

# ThreadSanitizer (TSan) — 仅在 GCC/Clang 下支持,与 ASAN 互斥
# 用法: cmake -DENABLE_TSAN=ON -DENABLE_SANITIZERS=OFF -DBUILD_TESTS=ON ...
# 配套: TSAN_OPTIONS=suppressions=${CMAKE_SOURCE_DIR}/cmake/tsan_suppressions.txt
if(ENABLE_TSAN)
    if(MSVC)
        message(FATAL_ERROR "ThreadSanitizer is not supported on MSVC. Use Linux/macOS with GCC or Clang.")
    endif()
    if(ENABLE_SANITIZERS)
        message(FATAL_ERROR "ENABLE_TSAN and ENABLE_SANITIZERS are mutually exclusive. Disable ENABLE_SANITIZERS.")
    endif()
    add_compile_options(-fsanitize=thread -g -O1 -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
    message(STATUS "ThreadSanitizer enabled (compile + link)")
endif()

if(ENABLE_LTO OR ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
    if(ipo_supported)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
        message(STATUS "Link-Time Optimization enabled")
    else()
        message(WARNING "IPO/LTO not supported: ${ipo_output}")
    endif()
endif()

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_compile_options(--coverage -O0 -g)
        add_link_options(--coverage)

        find_program(LCOV_PATH lcov)
        find_program(GENHTML_PATH genhtml)
        find_program(GCOVR_PATH gcovr)

        # lcov + genhtml: HTML 报告 (一级方案)
        if(LCOV_PATH AND GENHTML_PATH)
            add_custom_target(coverage
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '${CMAKE_BINARY_DIR}/*' '*/test*' --output-file coverage.info.cleaned
                COMMAND ${GENHTML_PATH} -o coverage_report coverage.info.cleaned
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report (lcov + genhtml)"
            )

            add_custom_target(coverage-clean
                COMMAND ${LCOV_PATH} --directory . --zerocounters
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Cleaning coverage counters"
            )
        endif()

        # gcovr: Cobertura/XML + JSON + HTML 多格式报告 [v1.9.4 新增]
        # 与 lcov 互补,便于 CI 集成 (XML 可被 GitLab/Jenkins 解析)
        if(GCOVR_PATH)
            add_custom_target(coverage-xml
                COMMAND ${GCOVR_PATH}
                    --root ${CMAKE_SOURCE_DIR}
                    --build ${CMAKE_BINARY_DIR}
                    --xml-pretty
                    --output ${CMAKE_BINARY_DIR}/coverage.xml
                    --exclude '*/test*'
                    --exclude '/usr/*'
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating Cobertura XML coverage report (gcovr)"
            )

            add_custom_target(coverage-json
                COMMAND ${GCOVR_PATH}
                    --root ${CMAKE_SOURCE_DIR}
                    --build ${CMAKE_BINARY_DIR}
                    --json-pretty
                    --output ${CMAKE_BINARY_DIR}/coverage.json
                    --exclude '*/test*'
                    --exclude '/usr/*'
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating JSON coverage report (gcovr)"
            )

            add_custom_target(coverage-html
                COMMAND ${GCOVR_PATH}
                    --root ${CMAKE_SOURCE_DIR}
                    --build ${CMAKE_BINARY_DIR}
                    --html-details
                    --output ${CMAKE_BINARY_DIR}/coverage_gcovr/index.html
                    --exclude '*/test*'
                    --exclude '/usr/*'
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating HTML coverage report (gcovr)"
            )
        endif()

        if(NOT LCOV_PATH AND NOT GCOVR_PATH)
            message(WARNING "ENABLE_COVERAGE=ON but neither lcov nor gcovr found. "
                            "Install lcov or gcovr (pip install gcovr) to generate reports.")
        endif()
    else()
        message(WARNING "ENABLE_COVERAGE only supported on GCC. Current compiler: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2 /Ob2 /Oi /Ot /Oy /GL)
        add_link_options(/LTCG /OPT:REF /OPT:ICF)
    else()
        add_compile_options(-O3 -march=native -mtune=native)
        add_link_options(-Wl,--gc-sections)
    endif()
endif()

# 自动复制 Qt DLL 到可执行文件输出目录 (Windows)
# 这样运行测试时不需要手动设置环境变量
# 注意：函数定义在 if(WIN32) 之外，确保所有平台都能解析 copy_qt_dll() 调用。
# 在 Linux/macOS 上函数体为空操作（no-op），避免 "Unknown CMake command" 错误。
if(WIN32)
    get_target_property(QT6_CORE_LOCATION Qt6::Core IMPORTED_LOCATION)
    get_filename_component(QT6_BIN_DIR ${QT6_CORE_LOCATION} DIRECTORY)
    find_program(WINDEPLOYQT_EXECUTABLE NAMES windeployqt HINTS "${QT6_BIN_DIR}")
endif()

function(windeployqt_post_build TARGET_NAME IS_TEST)
    if(NOT WIN32)
        return()
    endif()

    if(NOT WINDEPLOYQT_EXECUTABLE)
        message(WARNING "windeployqt not found! Set WINDEPLOYQT_EXECUTABLE or ensure Qt bin dir is in PATH")
        return()
    endif()

    # v3.0.0: 用文件锁串行化 windeployqt 调用。
    # 根因: -jN 并行构建时, 多个 target 的 windeployqt 同时向同一输出目录
    #       (tests/ 或 examples/) 复制共享 Qt DLL (Qt6Core/Qt6Network 等),
    #       产生 "Cannot create ... for output" 写竞争 (Build System Defect)。
    # 修复: 通过 cmake/deploy_qt_locked.cmd 用原子目录锁串行化, 锁按输出目录
    #       隔离 (同目录 target 共享一把锁, 不同目录互不阻塞)。
    set(_LOCK_DIR "$<TARGET_FILE_DIR:${TARGET_NAME}>/.deploy.lock")
    set(_DEPLOY_SCRIPT "${CMAKE_SOURCE_DIR}/cmake/deploy_qt_locked.cmd")

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${_DEPLOY_SCRIPT}"
            "${_LOCK_DIR}"
            "${WINDEPLOYQT_EXECUTABLE}"
            "--debug"
            "--no-translations"
            "--no-system-d3d-compiler"
            $<$<BOOL:${IS_TEST}>:--test>
            "$<TARGET_FILE:${TARGET_NAME}>"
        COMMENT "Deploying Qt dependencies for ${TARGET_NAME} (locked)"
    )

    # Copy qoffscreen.dll platform plugin (missing from windeployqt output)
    get_filename_component(QT6_PLUGINS_DIR "${QT6_BIN_DIR}/../plugins" ABSOLUTE)
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/platforms"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${QT6_PLUGINS_DIR}/platforms/qoffscreen.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/platforms/qoffscreen.dll"
        COMMENT "Copying qoffscreen.dll platform plugin for ${TARGET_NAME}"
    )

endfunction()

# ============================================================================
# 子模块定义 — SoulCoreKit 四层架构
# ============================================================================
# 架构定位: 面向 CS + BS 场景的 C++/Qt 通用应用基础平台
# 分层原则: Core → Infrastructure → Extensions → Adapters
#           上层可依赖下层，下层禁止依赖上层，严禁循环依赖
#
#   SoulCoreKit
#        │
#   ┌────┼──────────────────────┐
#   │    │                      │
#  Core  Infrastructure    Extensions (Adapters)
#   │    │                      │
# CS/BS  CS/BS                 可选企业能力
# 共用   共用基础设施           不污染核心架构
#
# 模块间依赖关系见 docs/01_architecture.md 的 Module Dependency Graph。

# ============================================================================
# Layer 0: Core 层 — CS / BS 共用的极稳定核心
# ============================================================================
# 设计原则: 零业务绑定，CS/BS 统一使用，API 极简稳定
# 模块: core, di, logging, async, event

# C01. Core — 核心类型与基础设施，无内部依赖 (仅 Qt)
include(cmake/modules/core.cmake)

# v1.9.4: 从 CMake PROJECT_VERSION 自动生成版本号头文件
configure_file(
    ${PROJECT_SOURCE_DIR}/include/soul/core/version_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/soul/core/version_config.h
    @ONLY
)

# C02. DI — 依赖注入容器 (Singleton/Scoped/Transient)，依赖 core
include(cmake/modules/di.cmake)

# C03. Logging — 日志系统 (spdlog 后端)，依赖 core + spdlog
include(cmake/modules/logging.cmake)

# C04. Async — 异步任务框架 (ThreadPool/Future/Promise/Coroutine)，依赖 core + logging (PRIVATE)
include(cmake/modules/async.cmake)

# C05. Event — 事件总线 (EventBus/TypedEventBus)，依赖 core + async (PRIVATE) + logging (PRIVATE)
include(cmake/modules/event.cmake)

# ============================================================================
# Layer 1: Infrastructure 层 — CS / BS 共用基础设施
# ============================================================================
# 设计原则: 提供 CS/BS 都需要的通用能力，统一抽象、多后端适配
# 模块: network, data, database, orm, storage, cache, configuration, observability, validation, base, utils

# === 通信基础设施 ===
# I01. Network Core — 网络核心抽象层 (Adapter/Codec/Monitor)，依赖 core + logging
include(cmake/modules/network_core.cmake)

# I02. Network Policy — 网络策略 (熔断/重试/限流/心跳/超时)，依赖 core + network_core
include(cmake/modules/network_policy.cmake)

# I03. Network HTTP — HTTP 适配器与拦截器链，依赖 core + network_core + network_policy
include(cmake/modules/network_http.cmake)

# I04. Network Protocol — 多协议适配 (TCP/WebSocket/MQTT/Bluetooth/Serial/NamedPipe)，依赖 core + network_core
include(cmake/modules/network_protocol.cmake)

# I05. Network — 网络通信聚合 (HttpClient/WebSocket/TcpClient/Downloader/Uploader)
# 依赖 core + network_core + network_policy + network_http + network_protocol + Qt::Network + Qt::WebSockets
include(cmake/modules/network.cmake)

# === 数据基础设施 ===
# I06. Data — 数据访问抽象 (IRepository/MemoryRepository/QueryCache/ORM Reflection/Transaction)，依赖 core + Qt::Sql
include(cmake/modules/data.cmake)

# I07. Database — 数据库实现聚合 (IDatabaseDriver/BaseRepository/Migration)
# MySQL 作为一等公民 (BS Server)，SQLite 服务 CS Client 本地数据
include(cmake/modules/database.cmake)

# I08. ORM — 对象关系映射 (QueryWrapper/SqlDialect/MigrationManager/CachedRepository)，依赖 core + logging + data + cache
include(cmake/modules/orm.cmake)

# I09. Storage — KV 存储与持久化 (Memory/File/SQLite/Settings)，依赖 core + logging + Qt::Sql + nlohmann_json
include(cmake/modules/storage.cmake)

# I10. Cache — 缓存抽象 (Memory/Disk/MultiLevel/SizeEstimator)，依赖 core + logging (PRIVATE)
include(cmake/modules/cache.cmake)

# === 通用基础设施 ===
# I11. Base — 基础类与抽象 (BaseObject/BaseManager/BaseService)，依赖 core + data
include(cmake/modules/base.cmake)

# I12. Utils — 工具函数集 (Clipboard/Crypto/File/JSON/String/XML/Process/Compress/Datetime/Image)，依赖 core + Qt::Xml + nlohmann_json
include(cmake/modules/utils.cmake)

# I13. Configuration — 配置管理 (JSON/INI 多源 + 环境变量 + 热加载 + Profile 隔离 + ConfigCenter + Schema)，依赖 core + utils + logging
include(cmake/modules/configuration.cmake)

# I14. Observability — 可观测性 (Metrics/Tracing/HealthCheck/ResourcePool/OtlpExporter/PrometheusExporter)，依赖 core + logging + async + data + network
include(cmake/modules/observability.cmake)

# I15. Validation — 数据校验 (Validator/注解式校验)，依赖 core
include(cmake/modules/validation.cmake)

# ============================================================================
# Layer 2: Extensions 层 — 企业级扩展能力 (可选)
# ============================================================================
# 设计原则: 保留抽象，不过度实现，不污染核心架构
# 模块: rpc, mq, auth, plugin, scheduler, server, aop

# E01. RPC — RPC 框架 (RpcClient/RpcServer/ServiceDispatcher/ClientProxy/ServiceDiscovery)
# Transport 抽象: HTTP / WebSocket / TCP 皆可为 Transport，gRPC 作为 Adapter
include(cmake/modules/rpc.cmake)

# E02. MQ — 消息队列 (MessageBus 抽象)
# InMemory 核心实现 + RabbitMQ/Kafka/RocketMQ 作为 Adapter
include(cmake/modules/mq.cmake)

# RabbitMQ 真实后端(amqpcpp): 可选编译 (Adapter 层)
option(ENABLE_RABBITMQ "Enable real RabbitMQ backend via amqpcpp (Linux/macOS only, requires network)" OFF)
if(ENABLE_RABBITMQ)
    if(WIN32)
        message(FATAL_ERROR "ENABLE_RABBITMQ is not supported on Windows.")
    endif()
    message(STATUS "ENABLE_RABBITMQ=ON: fetching amqpcpp and enabling real AMQP backend")
    include(FetchContent)
    FetchContent_Declare(
        amqpcpp
        GIT_REPOSITORY https://github.com/CopernicaMarketingSoftware/AMQP-CPP.git
        GIT_TAG        v4.3.27
        GIT_SHALLOW    TRUE
    )
    set(AMQP-CPP_BUILD_SHARED OFF CACHE BOOL "" FORCE)
    set(AMQP-CPP_LINUX_TCP ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(amqpcpp)
    target_sources(soul_mq PRIVATE
        include/soul/mq/amqpcpp_backend.h
        src/soul/mq/amqpcpp_backend.cpp
    )
    target_link_libraries(soul_mq PRIVATE amqpcpp)
    target_compile_definitions(soul_mq PUBLIC SOUL_ENABLE_RABBITMQ=1)
endif()

# E03. Auth — 认证授权 (AuthManager/TokenManager/Permission/OAuth2/OIDC)，依赖 core + network + storage + utils
include(cmake/modules/auth.cmake)

# E04. Plugin — 插件系统 (C-ABI/DLL动态加载/PluginManager)，依赖 core + di + logging
include(cmake/modules/plugin.cmake)

# E05. Scheduler — 定时任务调度 (ScheduledTask/CronTrigger)，依赖 core + di
include(cmake/modules/scheduler.cmake)

# E06. Server — 嵌入式 HTTP/WebSocket Server (BS 场景核心入口)，依赖 core + logging + network + observability + scheduler
include(cmake/modules/server.cmake)

# E07. AOP — 切面编程 (AspectWeaver/Before/After/Around/Pointcut/JoinPoint)，依赖 core + logging
include(cmake/modules/aop.cmake)

# ============================================================================
# Layer 3: Application 层 — 业务场景适配
# ============================================================================
# 设计原则: 面向具体应用场景的适配层，CS 和 BS 各自独立
# 模块: application (应用上下文), ui (CS Client), cs (CS 架构)

# A03. Application — 应用上下文与生命周期管理，依赖 core + di + configuration + cs
include(cmake/modules/application.cmake)

# A01. UI — 声明式 UI 组件库 (30+ Widgets/Theme/Style/玻璃效果)，依赖 core + base + configuration + Qt::Widgets + Qt::Gui
# 仅 CS Client 使用，BS/Headless 项目不链接此模块
include(cmake/modules/ui.cmake)

# A02. CS — CS 架构核心 (CsController/CsRouter/CsViewModel/CsWindowManager/CsAdminPanel)
# 依赖 core + di + ui + data + application
include(cmake/modules/cs.cmake)

# ============================================================================
# 四层架构依赖规则强制检查 [v2.6.0]
# ============================================================================
# 在模块全部定义之后、聚合库创建之前执行架构检查。
# 违规项将导致 FATAL_ERROR，阻断构建。
include(cmake/SoulCoreKitArchitecture.cmake)
sc_check_architecture()

# ============================================================================

# ============================================================================
# 顶层聚合 INTERFACE 库
# ============================================================================
# SoulCoreKit 定位: 面向 CS + BS 场景的 C++/Qt 通用应用基础平台
#
# 聚合策略:
#   SoulCoreKit        — Core + Infrastructure + Extensions (不含 UI/CS)
#                         适用: BS Web Backend / CLI 工具 / Headless 服务 / CS Server
#   SoulCoreKitUi      — SoulCoreKit + UI 组件库
#                         适用: CS Qt Desktop Client
#   SoulCoreKitFull    — 全量聚合 (含 CS 架构层)
#                         适用: 完整 CS 应用开发
#
# 使用示例:
#   # BS Web Backend / CLI 工具
#   target_link_libraries(MyServer PRIVATE SoulCoreKit)
#
#   # CS Qt Desktop Client
#   target_link_libraries(MyClient PRIVATE SoulCoreKit SoulCoreKitUi)
#
#   # 完整 CS 应用
#   target_link_libraries(MyApp PRIVATE SoulCoreKitFull)

# SoulCoreKit: Core + Infrastructure + Extensions (不含 Application 层)
add_library(SoulCoreKit INTERFACE)
target_link_libraries(SoulCoreKit INTERFACE
    # === Core 层 ===
    soul_core
    soul_di
    soul_logging
    soul_async
    soul_event
    # === Infrastructure 层 ===
    soul_network
    soul_data
    soul_database
    soul_orm
    soul_storage
    soul_cache
    soul_base
    soul_utils
    soul_configuration
    soul_observability
    soul_validation
    # === Extensions 层 ===
    soul_rpc
    soul_mq
    soul_auth
    soul_plugin
    soul_scheduler
    soul_server
    soul_aop
)

# SoulCoreKitUi: 包含 Application 层 + UI 组件库 (CS Client 专用)
add_library(SoulCoreKitUi INTERFACE)
target_link_libraries(SoulCoreKitUi INTERFACE
    SoulCoreKit
    soul_application
    soul_ui
)

# SoulCoreKitFull: 全量聚合 (含 CS 架构层)
add_library(SoulCoreKitFull INTERFACE)
target_link_libraries(SoulCoreKitFull INTERFACE
    SoulCoreKit
    SoulCoreKitUi
    soul_cs
)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ============================================================================
# Build Options [v2.8.0 统一]
# ============================================================================
option(SOULCOREKIT_BUILD_BENCHMARKS "Build benchmark suite" OFF)
option(SOULCOREKIT_BUILD_EXAMPLES "Build examples" ON)
option(SOULCOREKIT_BUILD_TESTS "Build tests" ON)
option(SOULCOREKIT_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(SOULCOREKIT_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(SOULCOREKIT_ENABLE_TSAN "Enable ThreadSanitizer" OFF)

# 兼容旧 option 名称
set(BUILD_BENCHMARKS ${SOULCOREKIT_BUILD_BENCHMARKS})
set(BUILD_EXAMPLES ${SOULCOREKIT_BUILD_EXAMPLES})
set(BUILD_TESTS ${SOULCOREKIT_BUILD_TESTS})

# --- Sanitizer 配置 ---
if(SOULCOREKIT_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
    message(STATUS "AddressSanitizer enabled")
endif()

if(SOULCOREKIT_ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
    message(STATUS "UndefinedBehaviorSanitizer enabled")
endif()

if(SOULCOREKIT_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
    add_link_options(-fsanitize=thread)
    message(STATUS "ThreadSanitizer enabled")
endif()

# --- Benchmarks [v2.8.0 增强] ---
if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

include(GNUInstallDirs)

install(TARGETS SoulCoreKit SoulCoreKitUi SoulCoreKitFull
                soul_core soul_di soul_logging soul_async soul_event soul_application
                soul_network soul_network_core soul_network_policy soul_network_http soul_network_protocol
                soul_data soul_database soul_orm soul_storage soul_cache
                soul_base soul_utils soul_configuration soul_observability soul_validation
                soul_rpc soul_mq soul_auth soul_plugin soul_scheduler soul_server soul_aop
                soul_ui soul_cs
        EXPORT SoulCoreKitTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/soul
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)

install(EXPORT SoulCoreKitTargets
        FILE SoulCoreKitTargets.cmake
        NAMESPACE sc::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SoulCoreKit
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/SoulCoreKitConfig.cmake.in
    ${PROJECT_BINARY_DIR}/SoulCoreKitConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SoulCoreKit
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/SoulCoreKitConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES ${PROJECT_BINARY_DIR}/SoulCoreKitConfig.cmake
              ${PROJECT_BINARY_DIR}/SoulCoreKitConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SoulCoreKit
)

include(CPack)

set(CPACK_PACKAGE_NAME "SoulCoreKit")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "CS+BS C++/Qt universal application foundation platform")
set(CPACK_PACKAGE_VENDOR "SoulCore")
set(CPACK_PACKAGE_CONTACT "support@soulcore.io")

set(CPACK_PACKAGE_INSTALL_DIRECTORY "SoulCoreKit")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")

set(CPACK_COMPONENTS_ALL libraries headers)
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Headers")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    set(CPACK_NSIS_PACKAGE_NAME "SoulCoreKit")
    set(CPACK_NSIS_DISPLAY_NAME "SoulCoreKit ${PROJECT_VERSION}")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/soulcore/SoulCoreKit")
    set(CPACK_NSIS_CONTACT "${CPACK_PACKAGE_CONTACT}")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA "")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "")
elseif(UNIX)
    if(APPLE)
        set(CPACK_GENERATOR "ZIP;DragNDrop")
    else()
        set(CPACK_GENERATOR "ZIP;DEB;RPM")
        set(CPACK_DEBIAN_PACKAGE_NAME "soulcorekit")
        set(CPACK_DEBIAN_PACKAGE_VERSION ${PROJECT_VERSION})
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
        set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
        set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
        set(CPACK_DEBIAN_PACKAGE_DEPENDS "qt6-base (>= 6.2)")
        set(CPACK_RPM_PACKAGE_NAME "SoulCoreKit")
        set(CPACK_RPM_PACKAGE_VERSION ${PROJECT_VERSION})
        set(CPACK_RPM_PACKAGE_RELEASE 1)
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
        set(CPACK_RPM_PACKAGE_LICENSE "MIT")
        set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
        set(CPACK_RPM_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
        set(CPACK_RPM_PACKAGE_REQUIRES "qt6-base >= 6.2")
    endif()
else()
    set(CPACK_GENERATOR "ZIP")
endif()
