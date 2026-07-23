cmake_minimum_required(VERSION 3.16)
project(SoulCoreKit VERSION 1.3.0 LANGUAGES CXX)

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
option(ENABLE_LTO "Enable Link-Time Optimization" OFF)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_IPO "Enable Inter-Procedural Optimization" OFF)

set(QT6_DIR "" CACHE PATH "Qt6 CMake config directory (optional)")
if(QT6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${QT6_DIR}")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Sql Xml)

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
endif()

function(copy_qt_dll TARGET_NAME COPY_TEST_DLL)
    if(NOT WIN32)
        return()
    endif()

    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_BIN_DIR}/Qt6Core.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Core.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_BIN_DIR}/Qt6Network.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Network.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_BIN_DIR}/Qt6Sql.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Sql.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_BIN_DIR}/Qt6Widgets.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Widgets.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_BIN_DIR}/Qt6Gui.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Gui.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_BIN_DIR}/Qt6WebSockets.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6WebSockets.dll"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_BIN_DIR}/Qt6Xml.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Xml.dll"
        COMMENT "Copying Qt6 DLLs to ${TARGET_NAME} output directory"
    )

    if(COPY_TEST_DLL)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT6_BIN_DIR}/Qt6Test.dll"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Test.dll"
            COMMENT "Copying Qt6Test.dll to ${TARGET_NAME} output directory"
        )
    endif()

    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT6_BIN_DIR}/libgcc_s_seh-1.dll"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/libgcc_s_seh-1.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT6_BIN_DIR}/libstdc++-6.dll"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/libstdc++-6.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT6_BIN_DIR}/libwinpthread-1.dll"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/libwinpthread-1.dll"
            COMMENT "Copying MinGW runtime DLLs to ${TARGET_NAME} output directory"
        )
    endif()

    set(QT6_PLUGINS_DIR "${QT6_BIN_DIR}/../plugins")
    add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/sqldrivers"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${QT6_PLUGINS_DIR}/sqldrivers/qsqlite.dll"
            "$<TARGET_FILE_DIR:${TARGET_NAME}>/sqldrivers/qsqlite.dll"
        COMMENT "Copying Qt6 SQL drivers to ${TARGET_NAME} output directory"
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
)

set(SOUL_CONFIGURATION_SOURCES
    src/soul/configuration/json_configuration.cpp
    src/soul/configuration/ini_configuration.cpp
    src/soul/configuration/config_schema.cpp
    src/soul/configuration/config.cpp
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

set(SOUL_ASYNC_HEADERS
    include/soul/async/async_runner.h
    include/soul/async/task.h
    include/soul/async/future.h
    include/soul/async/promise.h
    include/soul/async/dispatcher.h
    include/soul/async/cancelable_task.h
    include/soul/async/thread_pool.h
    include/soul/async/task_runner.h
)

set(SOUL_ASYNC_SOURCES
    src/soul/async/async_runner.cpp
    src/soul/async/task.cpp
    src/soul/async/dispatcher.cpp
    src/soul/async/cancelable_task.cpp
    src/soul/async/thread_pool.cpp
    src/soul/async/task_runner.cpp
)

set(SOUL_EVENT_HEADERS
    include/soul/event/i_event.h
    include/soul/event/event_bus.h
    include/soul/event/typed_event_bus.h
    include/soul/event/event_priority.h
    include/soul/event/subscription.h
    include/soul/event/i_message_bus.h
    include/soul/event/qt_signal_adapter.h
)

set(SOUL_EVENT_SOURCES
    src/soul/event/event_bus.cpp
    src/soul/event/subscription.cpp
    src/soul/event/qt_signal_adapter.cpp
)

set(SOUL_AUTH_HEADERS
    include/soul/auth/auth_manager.h
    include/soul/auth/token_manager.h
    include/soul/auth/permission.h
    include/soul/auth/token.h
    include/soul/auth/user.h
    include/soul/auth/permissions.h
)

set(SOUL_AUTH_SOURCES
    src/soul/auth/auth_manager.cpp
    src/soul/auth/token_manager.cpp
    src/soul/auth/permission.cpp
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
)

set(SOUL_DATA_HEADERS
    include/soul/data/repository.h
    include/soul/data/memory_repository.h
    include/soul/data/transaction.h
    include/soul/data/database_driver.h
    include/soul/data/connection_pool.h
)

add_library(soul_data STATIC
    ${SOUL_DATA_HEADERS}
)

target_include_directories(soul_data PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_data PUBLIC
    Qt6::Core
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
)

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

add_library(soul_event STATIC
    ${SOUL_EVENT_HEADERS}
    ${SOUL_EVENT_SOURCES}
)

target_include_directories(soul_event PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(soul_event PUBLIC
    Qt6::Core
    soul_core
    soul_async
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
)

set(SOUL_MQ_HEADERS
    include/soul/mq/imq_producer.h
    include/soul/mq/imq_consumer.h
    include/soul/mq/imq_connection.h
    include/soul/mq/mq_factory.h
    include/soul/mq/rabbitmq/rabbitmq_connection.h
    include/soul/mq/rabbitmq/rabbitmq_producer.h
    include/soul/mq/rabbitmq/rabbitmq_consumer.h
    include/soul/mq/module.h
)

set(SOUL_MQ_SOURCES
    src/soul/mq/mq_factory.cpp
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

set(SOUL_ORM_HEADERS
    include/soul/orm/entity.h
    include/soul/orm/query_wrapper.h
    include/soul/orm/base_repository.h
    include/soul/orm/sqlite_repository.h
    include/soul/orm/entities.h
    include/soul/orm/module.h
)

set(SOUL_ORM_SOURCES
    src/soul/orm/query_wrapper.cpp
    src/soul/orm/module.cpp
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

target_link_libraries(soul_plugin PUBLIC
    Qt6::Core
    soul_core
    soul_di
)

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
    soul_async
    soul_event
    soul_auth
    soul_mq
    soul_orm
    soul_ui
    soul_di
    soul_plugin
)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
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
        copy_qt_dll(benchmark_thread_pool FALSE)
        copy_qt_dll(benchmark_cache FALSE)
        copy_qt_dll(benchmark_connection_pool FALSE)
    endif()
endif()

include(GNUInstallDirs)

install(TARGETS SoulCoreKit soul_core soul_data soul_base soul_logging soul_network
                soul_utils soul_configuration soul_storage soul_async
                soul_event soul_auth soul_mq soul_orm soul_ui soul_di soul_plugin
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
