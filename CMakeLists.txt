cmake_minimum_required(VERSION 3.16)
project(SoulCoreKit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

option(BUILD_TESTS "Build test suite" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_DOCS "Build documentation" OFF)

# Qt6 CMake 配置路径 - 用户可通过 QT6_DIR 或 CMAKE_PREFIX_PATH 指定
# 在 CI/CD 环境中，Qt 通常通过包管理器或 install-qt-action 安装，不需要手动设置
set(QT6_DIR "" CACHE PATH "Qt6 CMake config directory (optional)")
if(QT6_DIR)
    list(APPEND CMAKE_PREFIX_PATH "${QT6_DIR}")
endif()

# 查找 Qt6 组件
find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Sql Widgets Xml)

# 自动复制 Qt DLL 到可执行文件输出目录 (Windows)
# 这样运行测试时不需要手动设置环境变量
if(WIN32)
    get_target_property(QT6_CORE_LOCATION Qt6::Core IMPORTED_LOCATION)
    get_filename_component(QT6_BIN_DIR ${QT6_CORE_LOCATION} DIRECTORY)
    
    function(copy_qt_dll TARGET_NAME COPY_TEST_DLL)
        add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT6_BIN_DIR}/Qt6Core.dll"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Core.dll"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${QT6_BIN_DIR}/Qt6Core5Compat.dll"
                "$<TARGET_FILE_DIR:${TARGET_NAME}>/Qt6Core5Compat.dll"
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
    endfunction()
endif()

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
    include/soul/base/base_repository.h
    include/soul/base/base_widget.h
    include/soul/base/base_dialog.h
    include/soul/base/base_view_model.h
)

set(SOUL_BASE_SOURCES
    src/soul/base/base_object.cpp
    src/soul/base/base_manager.cpp
    src/soul/base/base_service.cpp
    src/soul/base/base_repository.cpp
    src/soul/base/base_widget.cpp
    src/soul/base/base_dialog.cpp
    src/soul/base/base_view_model.cpp
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
    include/soul/network/http_request.h
    include/soul/network/http_response.h
    include/soul/network/i_interceptor.h
    include/soul/network/retry_policy.h
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
    include/soul/network/policy/heartbeat_policy.h
    include/soul/network/interceptor/i_interceptor.h
    include/soul/network/factory/network_factory.h
    include/soul/network/codec/icodec.h
    include/soul/network/codec/json_codec.h
    include/soul/network/codec/codec_factory.h
    include/soul/network/monitor/metrics.h
    include/soul/network/monitor/monitor.h
    include/soul/network/pool/connection_pool.h
    include/soul/network/http/http_client_adapter.h
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
    src/soul/network/policy/heartbeat_policy.cpp
    src/soul/network/interceptor/logging_interceptor.cpp
    src/soul/network/interceptor/auth_interceptor.cpp
    src/soul/network/codec/json_codec.cpp
    src/soul/network/codec/codec_factory.cpp
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
    include/soul/storage/repository.h
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
)

set(SOUL_UI_SOURCES
    src/soul/ui/base_view.cpp
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
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(soul_core PUBLIC
    Qt6::Core
)

add_library(soul_base STATIC
    ${SOUL_BASE_HEADERS}
    ${SOUL_BASE_SOURCES}
)

target_include_directories(soul_base PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(soul_base PUBLIC
    Qt6::Core
    Qt6::Widgets
    soul_core
    soul_ui
)

add_library(soul_logging STATIC
    ${SOUL_LOGGING_HEADERS}
    ${SOUL_LOGGING_SOURCES}
)

target_include_directories(soul_logging PUBLIC
    ${PROJECT_SOURCE_DIR}/include
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
    ${PROJECT_SOURCE_DIR}/include
)

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
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(soul_utils PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Xml
    soul_core
)

add_library(soul_configuration STATIC
    ${SOUL_CONFIGURATION_HEADERS}
    ${SOUL_CONFIGURATION_SOURCES}
)

target_include_directories(soul_configuration PUBLIC
    ${PROJECT_SOURCE_DIR}/include
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
    ${PROJECT_SOURCE_DIR}/include
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
    ${PROJECT_SOURCE_DIR}/include
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
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(soul_event PUBLIC
    Qt6::Core
    soul_core
)

add_library(soul_auth STATIC
    ${SOUL_AUTH_HEADERS}
    ${SOUL_AUTH_SOURCES}
)

target_include_directories(soul_auth PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(soul_auth PUBLIC
    Qt6::Core
    soul_core
    soul_network
    soul_storage
    soul_utils
)

add_library(soul_ui STATIC
    ${SOUL_UI_HEADERS}
    ${SOUL_UI_SOURCES}
)

target_include_directories(soul_ui PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(soul_ui PUBLIC
    Qt6::Core
    Qt6::Widgets
    soul_core
    soul_configuration
)

add_library(SoulCoreKit INTERFACE)

target_link_libraries(SoulCoreKit INTERFACE
    soul_core
    soul_base
    soul_logging
    soul_network
    soul_utils
    soul_configuration
    soul_storage
    soul_async
    soul_event
    soul_auth
    soul_ui
)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()