cmake_minimum_required(VERSION 3.16)
project(SoulCoreKit VERSION 1.9.2 LANGUAGES CXX)

include(GenerateExportHeader)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

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
        
        if(LCOV_PATH AND GENHTML_PATH)
            add_custom_target(coverage
                COMMAND ${LCOV_PATH} --directory . --capture --output-file coverage.info
                COMMAND ${LCOV_PATH} --remove coverage.info '/usr/*' '${CMAKE_BINARY_DIR}/*' '*/test*' --output-file coverage.info.cleaned
                COMMAND ${GENHTML_PATH} -o coverage_report coverage.info.cleaned
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Generating code coverage report"
            )
            
            add_custom_target(coverage-clean
                COMMAND ${LCOV_PATH} --directory . --zerocounters
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
                COMMENT "Cleaning coverage counters"
            )
        endif()
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

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND "${WINDEPLOYQT_EXECUTABLE}"
            "--debug"
            "--no-translations"
            "--no-system-d3d-compiler"
            $<$<BOOL:${IS_TEST}>:--test>
            "$<TARGET_FILE:${TARGET_NAME}>"
        COMMENT "Deploying Qt dependencies for ${TARGET_NAME}"
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

set(SOUL_CORE_HEADERS
    include/soul/core/result.h
    include/soul/core/error.h
    include/soul/core/interface.h
    include/soul/core/factory.h
    include/soul/core/singleton.h
    include/soul/core/version.h
    include/soul/core/time.h
    include/soul/core/uuid.h
    include/soul/core/environment.h
    include/soul/core/platform.h
    include/soul/core/application.h
    include/soul/core/module.h
    include/soul/core/scaffold.h
    include/soul/core/module_registry.h
    include/soul/core/crash_handler.h
    include/soul/core/benchmark.h
)

set(SOUL_DI_HEADERS
    include/soul/di/di_global.h
    include/soul/di/container.h
    include/soul/di/module.h
)

set(SOUL_DI_SOURCES
    src/soul/di/module.cpp
)

set(SOUL_PLUGIN_HEADERS
    include/soul/plugin/plugin_global.h
    include/soul/plugin/iplugin.h
    include/soul/plugin/plugin_manager.h
    include/soul/plugin/module.h
)

set(SOUL_PLUGIN_SOURCES
    src/soul/plugin/plugin_manager.cpp
    src/soul/plugin/module.cpp
)

set(SOUL_CORE_SOURCES
    src/soul/core/version.cpp
    src/soul/core/time.cpp
    src/soul/core/uuid.cpp
    src/soul/core/environment.cpp
    src/soul/core/platform.cpp
    src/soul/core/application.cpp
    src/soul/core/scaffold.cpp
    src/soul/core/module_registry.cpp
)

set(SOUL_BASE_HEADERS
    include/soul/base/base_object.h
    include/soul/base/base_manager.h
    include/soul/base/base_service.h
)

set(SOUL_BASE_SOURCES
    src/soul/base/base_object.cpp
    src/soul/base/base_manager.cpp
    src/soul/base/base_service.cpp
)

set(SOUL_LOGGING_HEADERS
    include/soul/logging/log_level.h
    include/soul/logging/log_record.h
    include/soul/logging/i_sink.h
    include/soul/logging/log_formatter.h
    include/soul/logging/console_sink.h
    include/soul/logging/file_sink.h
    include/soul/logging/daily_file_sink.h
    include/soul/logging/callback_sink.h
    include/soul/logging/composite_sink.h
    include/soul/logging/logger.h
    include/soul/logging/log_macros.h
)

set(SOUL_LOGGING_SOURCES
    src/soul/logging/log_formatter.cpp
    src/soul/logging/console_sink.cpp
    src/soul/logging/file_sink.cpp
    src/soul/logging/daily_file_sink.cpp
    src/soul/logging/callback_sink.cpp
    src/soul/logging/composite_sink.cpp
    src/soul/logging/logger.cpp
)

set(SOUL_NETWORK_HEADERS
    include/soul/network/network_global.h
    include/soul/network/connection_manager.h
    include/soul/network/http_request.h
    include/soul/network/http_response.h
    include/soul/network/http_client.h
    include/soul/network/http_api.h
    include/soul/network/web_socket.h
    include/soul/network/tcp_client.h
    include/soul/network/session.h
    include/soul/network/downloader.h
    include/soul/network/uploader.h
    include/soul/network/network_error.h
    include/soul/network/cookie_jar.h
    include/soul/network/core/inetwork.h
    include/soul/network/core/network_base.h
    include/soul/network/core/network_adapter_base.h
    include/soul/network/core/network_message.h
    include/soul/network/core/network_state.h
    include/soul/network/policy/inetwork_policy.h
    include/soul/network/policy/timeout_policy.h
    include/soul/network/policy/retry_policy.h
    include/soul/network/policy/reconnect_policy.h
    include/soul/network/policy/heartbeat_policy.h
    include/soul/network/interceptor/i_interceptor.h
    include/soul/network/interceptor/logging_interceptor.h
    include/soul/network/interceptor/auth_interceptor.h
    include/soul/network/factory/network_factory.h
    include/soul/network/codec/icodec.h
    include/soul/network/codec/json_codec.h
    include/soul/network/codec/codec_factory.h
    include/soul/network/monitor/metrics.h
    include/soul/network/monitor/monitor.h
    include/soul/network/pool/connection_pool.h
    include/soul/network/http/http_client_adapter.h
    include/soul/network/http/http_interceptor.h
    include/soul/network/tcp/tcp_client_adapter.h
    include/soul/network/websocket/ws_client_adapter.h
    include/soul/network/mqtt/mqtt_client_adapter.h
    include/soul/network/bluetooth/bluetooth_client_adapter.h
    include/soul/network/serial/serial_port_adapter.h
    include/soul/network/namedpipe/named_pipe_adapter.h
)

set(SOUL_NETWORK_SOURCES
    src/soul/network/http_request.cpp
    src/soul/network/http_response.cpp
    src/soul/network/http_client.cpp
    src/soul/network/web_socket.cpp
    src/soul/network/tcp_client.cpp
    src/soul/network/session.cpp
    src/soul/network/downloader.cpp
    src/soul/network/uploader.cpp
    src/soul/network/network_error.cpp
    src/soul/network/cookie_jar.cpp
    src/soul/network/connection_manager.cpp
    src/soul/network/policy/timeout_policy.cpp
    src/soul/network/policy/retry_policy.cpp
    src/soul/network/policy/reconnect_policy.cpp
    src/soul/network/policy/heartbeat_policy.cpp
    src/soul/network/interceptor/logging_interceptor.cpp
    src/soul/network/interceptor/auth_interceptor.cpp
    src/soul/network/codec/json_codec.cpp
    src/soul/network/codec/codec_factory.cpp
    src/soul/network/factory/network_factory.cpp
    src/soul/network/monitor/metrics.cpp
    src/soul/network/monitor/monitor.cpp
    src/soul/network/pool/connection_pool.cpp
    src/soul/network/core/network_adapter_base.cpp
    src/soul/network/http/http_client_adapter.cpp
    src/soul/network/tcp/tcp_client_adapter.cpp
    src/soul/network/websocket/ws_client_adapter.cpp
    src/soul/network/mqtt/mqtt_client_adapter.cpp
    src/soul/network/bluetooth/bluetooth_client_adapter.cpp
    src/soul/network/serial/serial_port_adapter.cpp
    src/soul/network/namedpipe/named_pipe_adapter.cpp
)

set(SOUL_UTILS_HEADERS
    include/soul/utils/json/json_utils.h
    include/soul/utils/json/json_helper.h
    include/soul/utils/file/file_utils.h
    include/soul/utils/string/string_utils.h
    include/soul/utils/crypto/crypto_utils.h
    include/soul/utils/image/image_utils.h
    include/soul/utils/process/process_utils.h
    include/soul/utils/clipboard/clipboard_utils.h
    include/soul/utils/datetime/datetime_utils.h
    include/soul/utils/compress/compress_utils.h
    include/soul/utils/xml/xml_utils.h
)

set(SOUL_UTILS_SOURCES
    src/soul/utils/json/json_utils.cpp
    src/soul/utils/file/file_utils.cpp
    src/soul/utils/string/string_utils.cpp
    src/soul/utils/crypto/crypto_utils.cpp
    src/soul/utils/image/image_utils.cpp
    src/soul/utils/process/process_utils.cpp
    src/soul/utils/clipboard/clipboard_utils.cpp
    src/soul/utils/datetime/datetime_utils.cpp
    src/soul/utils/compress/compress_utils.cpp
    src/soul/utils/xml/xml_utils.cpp
)

set(SOUL_CONFIGURATION_HEADERS
    include/soul/configuration/iconfiguration.h
    include/soul/configuration/json_configuration.h
    include/soul/configuration/ini_configuration.h
    include/soul/configuration/config_schema.h
    include/soul/configuration/config.h
    include/soul/configuration/config_bind.h
    include/soul/configuration/remote_config.h
    include/soul/configuration/nacos_source.h
    include/soul/configuration/etcd_source.h
)

set(SOUL_CONFIGURATION_SOURCES
    src/soul/configuration/json_configuration.cpp
    src/soul/configuration/ini_configuration.cpp
    src/soul/configuration/config_schema.cpp
    src/soul/configuration/config.cpp
    src/soul/configuration/remote_config.cpp
    src/soul/configuration/nacos_source.cpp
    src/soul/configuration/etcd_source.cpp
)

set(SOUL_STORAGE_HEADERS
    include/soul/storage/istorage.h
    include/soul/storage/memory_storage.h
    include/soul/storage/file_storage.h
    include/soul/storage/sqlite_database.h
    include/soul/storage/cache.h
    include/soul/storage/i_serializer.h
    include/soul/storage/json_serializer.h
    include/soul/storage/settings.h
)

set(SOUL_STORAGE_SOURCES
    src/soul/storage/memory_storage.cpp
    src/soul/storage/file_storage.cpp
    src/soul/storage/sqlite_database.cpp
    src/soul/storage/cache.cpp
    src/soul/storage/json_serializer.cpp
    src/soul/storage/settings.cpp
)

set(SOUL_CACHE_HEADERS
    include/soul/cache/icache.h
    include/soul/cache/size_estimator.h
    include/soul/cache/memory_cache.h
    include/soul/cache/disk_cache.h
    include/soul/cache/multi_level_cache.h
)

set(SOUL_CACHE_SOURCES
    src/soul/cache/disk_cache.cpp
)

set(SOUL_ASYNC_HEADERS
    include/soul/async/async_runner.h
    include/soul/async/task.h
    include/soul/async/future.h
    include/soul/async/future_detail.h
    include/soul/async/promise.h
    include/soul/async/dispatcher.h
    include/soul/async/cancelable_task.h
    include/soul/async/thread_pool.h
    include/soul/async/task_runner.h
    include/soul/async/coroutine.h
)

set(SOUL_ASYNC_SOURCES
    src/soul/async/async_runner.cpp
    src/soul/async/task.cpp
    src/soul/async/dispatcher.cpp
    src/soul/async/cancelable_task.cpp
    src/soul/async/thread_pool.cpp
    src/soul/async/task_runner.cpp
    src/soul/async/future_detail.cpp
)

set(SOUL_EVENT_HEADERS
    include/soul/event/i_event.h
    include/soul/event/event_bus.h
    include/soul/event/typed_event_bus.h
    include/soul/event/typed_event_bus_detail.h
    include/soul/event/event_priority.h
    include/soul/event/subscription.h
    include/soul/event/i_message_bus.h
    include/soul/event/qt_signal_adapter.h
)

set(SOUL_EVENT_SOURCES
    src/soul/event/event_bus.cpp
    src/soul/event/subscription.cpp
    src/soul/event/qt_signal_adapter.cpp
    src/soul/event/typed_event_bus_detail.cpp
)

set(SOUL_AUTH_HEADERS
    include/soul/auth/auth_manager.h
    include/soul/auth/token_manager.h
    include/soul/auth/permission.h
    include/soul/auth/token.h
    include/soul/auth/user.h
    include/soul/auth/permissions.h
    include/soul/auth/oauth2.h
    include/soul/auth/oidc.h
    include/soul/auth/secure_storage.h
)

set(SOUL_AUTH_SOURCES
    src/soul/auth/auth_manager.cpp
    src/soul/auth/token_manager.cpp
    src/soul/auth/permission.cpp
    src/soul/auth/oauth2.cpp
    src/soul/auth/oidc.cpp
)

set(SOUL_UI_HEADERS
    include/soul/ui/i_view.h
    include/soul/ui/base_view.h
    include/soul/ui/style.h
    include/soul/ui/theme.h
    include/soul/ui/dialog.h
    include/soul/ui/toast.h
    include/soul/ui/icon.h
    include/soul/ui/icon_manager.h
    include/soul/ui/animation.h
    include/soul/ui/navigation.h
    include/soul/ui/page.h
    include/soul/ui/window.h
    include/soul/ui/loading.h
    include/soul/ui/empty_widget.h
    include/soul/ui/i_glass_effect.h
    include/soul/ui/glass_widget.h
    include/soul/ui/button.h
    include/soul/ui/card.h
    include/soul/ui/sidebar.h
    include/soul/ui/sidebar_hover_filter.h
    include/soul/ui/input.h
    include/soul/ui/switch.h
    include/soul/ui/checkbox.h
    include/soul/ui/slider.h
    include/soul/ui/scroll_bar.h
    include/soul/ui/tab_bar.h
    include/soul/ui/progress.h
    include/soul/ui/spinner.h
    include/soul/ui/tool_tip.h
    include/soul/ui/avatar.h
    include/soul/ui/badge.h
    include/soul/ui/dropdown.h
    include/soul/ui/soul_ui.h
    include/soul/ui/base_widget.h
    include/soul/ui/base_dialog.h
    include/soul/ui/base_view_model.h
)

set(SOUL_UI_SOURCES
    src/soul/ui/base_view.cpp
    src/soul/ui/base_widget.cpp
    src/soul/ui/base_dialog.cpp
    src/soul/ui/base_view_model.cpp
    src/soul/ui/style.cpp
    src/soul/ui/theme.cpp
    src/soul/ui/dialog.cpp
    src/soul/ui/toast.cpp
    src/soul/ui/icon.cpp
    src/soul/ui/icon_manager.cpp
    src/soul/ui/animation.cpp
    src/soul/ui/navigation.cpp
    src/soul/ui/page.cpp
    src/soul/ui/window.cpp
    src/soul/ui/loading.cpp
    src/soul/ui/empty_widget.cpp
    src/soul/ui/glass_widget.cpp
    src/soul/ui/button.cpp
    src/soul/ui/card.cpp
    src/soul/ui/sidebar.cpp
    src/soul/ui/sidebar_hover_filter.cpp
    src/soul/ui/input.cpp
    src/soul/ui/switch.cpp
    src/soul/ui/checkbox.cpp
    src/soul/ui/slider.cpp
    src/soul/ui/scroll_bar.cpp
    src/soul/ui/tab_bar.cpp
    src/soul/ui/progress.cpp
    src/soul/ui/spinner.cpp
    src/soul/ui/tool_tip.cpp
    src/soul/ui/avatar.cpp
    src/soul/ui/badge.cpp
    src/soul/ui/dropdown.cpp
)

add_library(soul_core STATIC
    ${SOUL_CORE_HEADERS}
    ${SOUL_CORE_SOURCES}
)

target_include_directories(soul_core PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_core PUBLIC
    Qt6::Core
    nlohmann_json::nlohmann_json
)

set(SOUL_DATA_HEADERS
    include/soul/data/repository.h
    include/soul/data/memory_repository.h
    include/soul/data/transaction.h
    include/soul/data/database_driver.h
    include/soul/data/database_driver_base.h
    include/soul/data/connection_pool.h
)

set(SOUL_DATA_SOURCES
    src/soul/data/database_driver.cpp
    src/soul/data/memory_repository.cpp
    src/soul/data/transaction.cpp
)

add_library(soul_data STATIC
    ${SOUL_DATA_HEADERS}
    ${SOUL_DATA_SOURCES}
)

target_include_directories(soul_data PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_data PUBLIC
    Qt6::Core
    Qt6::Sql
    soul_core
)

add_library(soul_base STATIC
    ${SOUL_BASE_HEADERS}
    ${SOUL_BASE_SOURCES}
)

target_include_directories(soul_base PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_base PUBLIC
    Qt6::Core
    soul_core
    soul_data
)

add_library(soul_logging STATIC
    ${SOUL_LOGGING_HEADERS}
    ${SOUL_LOGGING_SOURCES}
)

target_include_directories(soul_logging PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_logging PUBLIC
    Qt6::Core
    soul_core
)

add_library(soul_network STATIC
    ${SOUL_NETWORK_HEADERS}
    ${SOUL_NETWORK_SOURCES}
)

target_include_directories(soul_network PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# 根据是否为共享库设置不同的编译宏
if(BUILD_SHARED_LIBS)
    target_compile_definitions(soul_network PRIVATE SC_NETWORK_LIBRARY)
else()
    target_compile_definitions(soul_network PUBLIC SC_NETWORK_STATIC_LIB)
endif()

target_link_libraries(soul_network PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::WebSockets
    soul_core
    soul_logging
)

add_library(soul_utils STATIC
    ${SOUL_UTILS_HEADERS}
    ${SOUL_UTILS_SOURCES}
)

target_include_directories(soul_utils PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_utils PUBLIC
    Qt6::Core
    Qt6::Xml
    soul_core
    nlohmann_json::nlohmann_json
)

# compress_utils.cpp uses zlib deflate/inflate directly.
# On Linux: link system zlib explicitly.
# On Windows/macOS: Qt6::Core provides zlib symbols transitively (no explicit link needed).
if(UNIX AND NOT APPLE)
    target_link_libraries(soul_utils PRIVATE ZLIB::ZLIB)
endif()

if(TARGET Qt6::Gui)
    target_link_libraries(soul_utils PUBLIC Qt6::Gui)
endif()

add_library(soul_configuration STATIC
    ${SOUL_CONFIGURATION_HEADERS}
    ${SOUL_CONFIGURATION_SOURCES}
)

target_include_directories(soul_configuration PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_configuration PUBLIC
    Qt6::Core
    Qt6::Network
    soul_core
    soul_utils
    soul_logging
)

add_library(soul_storage STATIC
    ${SOUL_STORAGE_HEADERS}
    ${SOUL_STORAGE_SOURCES}
)

target_include_directories(soul_storage PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_storage PUBLIC
    Qt6::Core
    Qt6::Sql
    soul_core
)

add_library(soul_cache STATIC
    ${SOUL_CACHE_HEADERS}
    ${SOUL_CACHE_SOURCES}
)

target_include_directories(soul_cache PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_cache PUBLIC
    Qt6::Core
    soul_core
)
target_link_libraries(soul_cache PRIVATE
    soul_logging
)

add_library(soul_async STATIC
    ${SOUL_ASYNC_HEADERS}
    ${SOUL_ASYNC_SOURCES}
)

target_include_directories(soul_async PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_async PUBLIC
    Qt6::Core
    soul_core
)

# soul_logging 改为 PRIVATE:future.h/task_runner.h 已通过 future_detail.cpp
# 隔离 Logger 调用,不再传递依赖 logger.h 到下游 TU。
target_link_libraries(soul_async PRIVATE
    soul_logging
)

# C++20 Coroutine support (experimental)
if(ENABLE_CXX20)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "11.0")
        message(FATAL_ERROR "GCC 11+ required for C++20 coroutines")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
        message(FATAL_ERROR "Clang 14+ required for C++20 coroutines")
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.29")
        message(FATAL_ERROR "MSVC 2019 16.11+ required for C++20 coroutines")
    endif()

    target_compile_definitions(soul_async PUBLIC SOUL_ENABLE_CXX20)
    target_compile_features(soul_async PUBLIC cxx_std_20)
endif()

add_library(soul_event STATIC
    ${SOUL_EVENT_HEADERS}
    ${SOUL_EVENT_SOURCES}
)

target_include_directories(soul_event PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# soul_async 改为 PRIVATE:typed_event_bus.h 已通过 typed_event_bus_detail.cpp
# 隔离 ThreadPool::instance().start() 调用,不再传递依赖 thread_pool.h 到下游 TU。
# soul_logging 为 PRIVATE:typed_event_bus_detail.cpp 使用 Logger。
target_link_libraries(soul_event PUBLIC
    Qt6::Core
    soul_core
)
target_link_libraries(soul_event PRIVATE
    soul_async
    soul_logging
)

add_library(soul_auth STATIC
    ${SOUL_AUTH_HEADERS}
    ${SOUL_AUTH_SOURCES}
)

target_include_directories(soul_auth PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_auth PUBLIC
    Qt6::Core
    soul_core
    soul_network
    soul_storage
    soul_utils
    nlohmann_json::nlohmann_json
)

set(SOUL_MQ_HEADERS
    include/soul/mq/imq_producer.h
    include/soul/mq/imq_consumer.h
    include/soul/mq/imq_connection.h
    include/soul/mq/mq_factory.h
    include/soul/mq/iamqp_backend.h
    include/soul/mq/inmemory_amqp_backend.h
    include/soul/mq/rabbitmq/rabbitmq_connection.h
    include/soul/mq/rabbitmq/rabbitmq_producer.h
    include/soul/mq/rabbitmq/rabbitmq_consumer.h
    include/soul/mq/module.h
)

set(SOUL_MQ_SOURCES
    src/soul/mq/mq_factory.cpp
    src/soul/mq/inmemory_amqp_backend.cpp
    src/soul/mq/rabbitmq/rabbitmq_connection.cpp
    src/soul/mq/rabbitmq/rabbitmq_producer.cpp
    src/soul/mq/rabbitmq/rabbitmq_consumer.cpp
    src/soul/mq/module.cpp
)

add_library(soul_mq STATIC
    ${SOUL_MQ_HEADERS}
    ${SOUL_MQ_SOURCES}
)

target_include_directories(soul_mq PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_mq PUBLIC
    Qt6::Core
    Qt6::Network
    soul_core
    soul_logging
    soul_async
)

# RabbitMQ 真实后端(amqpcpp): 可选编译
# 启用方式: cmake -DENABLE_RABBITMQ=ON ...
# 平台限制: amqpcpp 的 linux_tcp 仅支持 Linux/macOS;
#   Windows 平台需使用 Boost.Asio 后端(暂未集成,计划 v1.8.0)
#   Windows 用户请使用 InMemory 后端(默认)进行开发测试
# 详见 docs/v1.7.0/06_mq_real_integration_design.md
option(ENABLE_RABBITMQ "Enable real RabbitMQ backend via amqpcpp (Linux/macOS only, requires network)" OFF)
if(ENABLE_RABBITMQ)
    if(WIN32)
        message(FATAL_ERROR
            "ENABLE_RABBITMQ is not supported on Windows. "
            "amqpcpp linux_tcp requires Linux/macOS. "
            "Please use InMemory backend (default) on Windows, "
            "or build on Linux/macOS for production RabbitMQ integration. "
            "Windows Boost.Asio backend is planned for v1.8.0.")
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

set(SOUL_ORM_HEADERS
    include/soul/orm/entity.h
    include/soul/orm/column.h
    include/soul/orm/query_wrapper.h
    include/soul/orm/typed_query_wrapper.h
    include/soul/orm/sql_dialect.h
    include/soul/orm/base_repository.h
    include/soul/orm/sqlite_repository.h
    include/soul/orm/entities.h
    include/soul/orm/module.h
    include/soul/orm/read_write_repository.h
    include/soul/orm/code_generator.h
    include/soul/orm/cached_repository.h
    include/soul/orm/reflection.h
    include/soul/orm/reflection_macros.h
    include/soul/orm/migration.h
    include/soul/orm/query_result_cache.h
)

set(SOUL_ORM_SOURCES
    src/soul/orm/query_wrapper.cpp
    src/soul/orm/sql_dialect.cpp
    src/soul/orm/module.cpp
    src/soul/orm/migration.cpp
)

add_library(soul_orm STATIC
    ${SOUL_ORM_HEADERS}
    ${SOUL_ORM_SOURCES}
)

target_include_directories(soul_orm PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_orm PUBLIC
    Qt6::Core
    Qt6::Sql
    soul_core
    soul_logging
    soul_data
    soul_cache
)

# ============================================================================
# soul_observability 模块 — Metrics / Tracing / JsonSink
# ============================================================================

set(SOUL_OBSERVABILITY_HEADERS
    include/soul/observability/metrics.h
    include/soul/observability/tracing.h
    include/soul/observability/json_sink.h
    include/soul/observability/resource_pool_monitor.h
)

set(SOUL_OBSERVABILITY_SOURCES
    src/soul/observability/metrics.cpp
    src/soul/observability/tracing.cpp
    src/soul/observability/json_sink.cpp
    src/soul/observability/resource_pool_monitor.cpp
)

add_library(soul_observability STATIC
    ${SOUL_OBSERVABILITY_HEADERS}
    ${SOUL_OBSERVABILITY_SOURCES}
)

target_include_directories(soul_observability PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# v1.9.0: resource_pool_monitor 适配 ThreadPool / network::ConnectionPool /
# data::DbConnectionPool,因此依赖 async / network / data 模块。
# 注意: soul_network 依赖 soul_observability(用于 monitor/metrics),
# 为避免循环依赖,resource_pool_monitor 的三个适配器实现放在 .cpp 中,
# 头文件仅用前置声明,链接期解析。
target_link_libraries(soul_observability PUBLIC
    Qt6::Core
    Qt6::Network
    soul_core
    soul_logging
    soul_async
    soul_data
    soul_network
    nlohmann_json::nlohmann_json
)

# ============================================================================
# soul_aop 模块 — AOP 切面编程(v1.9.0 新增)
# 对标 SpringBoot AOP: JoinPoint / Pointcut / Advice(Before/After/Around) / AspectWeaver
# ============================================================================

set(SOUL_AOP_HEADERS
    include/soul/aop/aop.h
)

set(SOUL_AOP_SOURCES
    src/soul/aop/aop.cpp
)

add_library(soul_aop STATIC
    ${SOUL_AOP_HEADERS}
    ${SOUL_AOP_SOURCES}
)

target_include_directories(soul_aop PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_aop PUBLIC
    Qt6::Core
    soul_core
    soul_logging
)

# ============================================================================
# soul_scheduler 模块 — 定时任务框架(v1.9.1 新增)
# 对标 SpringBoot @Scheduled: 支持 cron 表达式 / fixedRate / fixedDelay
# ============================================================================

set(SOUL_SCHEDULER_HEADERS
    include/soul/scheduler/scheduled_task.h
    include/soul/scheduler/scheduler.h
    include/soul/scheduler/module.h
)

set(SOUL_SCHEDULER_SOURCES
    src/soul/scheduler/scheduled_task.cpp
    src/soul/scheduler/scheduler.cpp
    src/soul/scheduler/module.cpp
)

add_library(soul_scheduler STATIC
    ${SOUL_SCHEDULER_HEADERS}
    ${SOUL_SCHEDULER_SOURCES}
)

target_include_directories(soul_scheduler PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_scheduler PUBLIC
    Qt6::Core
    soul_core
    soul_di
)

# ============================================================================
# soul_server 模块 — 嵌入式 HTTP Server(v1.9.0 新增)
# 对标 SpringBoot 内嵌 Tomcat: 基于 QTcpServer 自研轻量 HTTP/1.1 Server
# ============================================================================

set(SOUL_SERVER_HEADERS
    include/soul/server/http_server.h
    include/soul/server/middleware.h
    include/soul/server/health.h
    include/soul/server/websocket_server.h
)

set(SOUL_SERVER_SOURCES
    src/soul/server/http_server.cpp
    src/soul/server/middleware.cpp
    src/soul/server/health.cpp
    src/soul/server/websocket_server.cpp
)

add_library(soul_server STATIC
    ${SOUL_SERVER_HEADERS}
    ${SOUL_SERVER_SOURCES}
)

target_include_directories(soul_server PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_server PUBLIC
    Qt6::Core
    Qt6::Network
    soul_core
    soul_logging
    nlohmann_json::nlohmann_json
)

set(SOUL_RPC_SOURCES
    src/soul/rpc/iserializer.cpp
    src/soul/rpc/irpc_transport.cpp
    src/soul/rpc/service_dispatcher.cpp
    src/soul/rpc/client_proxy.cpp
    src/soul/rpc/service_registry.cpp
    src/soul/rpc/http_transport.cpp
)

set(SOUL_RPC_HEADERS
    include/soul/rpc/iserializer.h
    include/soul/rpc/irpc_transport.h
    include/soul/rpc/service_dispatcher.h
    include/soul/rpc/client_proxy.h
    include/soul/rpc/service_registry.h
    include/soul/rpc/http_transport.h
    include/soul/soul_rpc.h
)

add_library(soul_rpc STATIC
    ${SOUL_RPC_SOURCES}
    ${SOUL_RPC_HEADERS}
)

target_include_directories(soul_rpc PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_rpc PUBLIC
    Qt6::Core
    Qt6::Network
    soul_core
    nlohmann_json::nlohmann_json
)

set_target_properties(soul_rpc PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${SOUL_RPC_HEADERS}"
)

add_library(soul_ui STATIC
    ${SOUL_UI_HEADERS}
    ${SOUL_UI_SOURCES}
)

target_include_directories(soul_ui PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_ui PUBLIC
    Qt6::Core
    soul_core
    soul_base
    soul_configuration
)

if(TARGET Qt6::Widgets)
    target_link_libraries(soul_ui PUBLIC Qt6::Widgets)
endif()

add_library(soul_di STATIC
    ${SOUL_DI_HEADERS}
    ${SOUL_DI_SOURCES}
)

target_include_directories(soul_di PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(soul_di PRIVATE SC_DI_LIBRARY)
else()
    target_compile_definitions(soul_di PUBLIC SC_DI_STATIC_LIB)
endif()

target_link_libraries(soul_di PUBLIC
    Qt6::Core
    soul_core
)

add_library(soul_plugin STATIC
    ${SOUL_PLUGIN_HEADERS}
    ${SOUL_PLUGIN_SOURCES}
)

target_include_directories(soul_plugin PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BUILD_SHARED_LIBS)
    target_compile_definitions(soul_plugin PRIVATE SC_PLUGIN_LIBRARY)
else()
    target_compile_definitions(soul_plugin PUBLIC SC_PLUGIN_STATIC_LIB)
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(soul_plugin PRIVATE -Wno-cast-function-type)
endif()

target_link_libraries(soul_plugin PUBLIC
    Qt6::Core
    soul_core
    soul_di
    soul_logging
)

# ============================================================================
# 顶层聚合 INTERFACE 库
# ============================================================================
# 设计原则: 严格遵循"轻量骨架"定位,SoulCoreKit 核心库默认不包含 UI 模块,
# 避免纯后端项目(CLI 工具/服务进程/Headless 服务)被迫引入 Qt Widgets/Gui 依赖。
# 需要 UI 的项目显式链接 SoulCoreKitUi。
add_library(SoulCoreKit INTERFACE)

target_link_libraries(SoulCoreKit INTERFACE
    soul_core
    soul_data
    soul_base
    soul_logging
    soul_network
    soul_utils
    soul_configuration
    soul_storage
    soul_cache
    soul_async
    soul_event
    soul_auth
    soul_mq
    soul_orm
    soul_rpc
    soul_observability
    soul_aop
    soul_scheduler
    soul_server
    soul_di
    soul_plugin
)

# SoulCoreKitUi: 包含 UI 模块的聚合库(可选)。
# 用法: target_link_libraries(MyApp PRIVATE SoulCoreKit SoulCoreKitUi)
add_library(SoulCoreKitUi INTERFACE)
target_link_libraries(SoulCoreKitUi INTERFACE
    SoulCoreKit
    soul_ui
)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(BUILD_BENCHMARKS "Build benchmark suite" OFF)

if(BUILD_BENCHMARKS)
    add_executable(benchmark_thread_pool benchmarks/benchmark_thread_pool.cpp)
    target_link_libraries(benchmark_thread_pool PRIVATE
        Qt6::Core
        soul_async
        soul_core
    )

    add_executable(benchmark_cache benchmarks/benchmark_cache.cpp)
    target_link_libraries(benchmark_cache PRIVATE
        Qt6::Core
        soul_storage
        soul_core
    )

    add_executable(benchmark_connection_pool benchmarks/benchmark_connection_pool.cpp)
    target_link_libraries(benchmark_connection_pool PRIVATE
        Qt6::Core
        Qt6::Network
        soul_network
        soul_core
    )

    if(WIN32)
        windeployqt_post_build(benchmark_thread_pool FALSE)
        windeployqt_post_build(benchmark_cache FALSE)
        windeployqt_post_build(benchmark_connection_pool FALSE)
    endif()
endif()

include(GNUInstallDirs)

install(TARGETS SoulCoreKit SoulCoreKitUi
                soul_core soul_data soul_base soul_logging soul_network
                soul_utils soul_configuration soul_storage soul_cache soul_async
                soul_event soul_auth soul_mq soul_orm soul_rpc soul_ui soul_di soul_plugin
                soul_observability soul_aop soul_scheduler soul_server
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
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "A comprehensive C++/Qt core framework for building scalable applications")
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
