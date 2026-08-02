find_package(Qt6 REQUIRED COMPONENTS Test Core Network)

add_executable(test_result
    test_result.cpp
)

target_link_libraries(test_result PRIVATE
    Qt6::Test
    Qt6::Core
    soul_core
)

windeployqt_post_build(test_result TRUE)

add_executable(test_event_bus
    test_event_bus.cpp
)

target_link_libraries(test_event_bus PRIVATE
    Qt6::Test
    Qt6::Core
    soul_event
    soul_core
)

windeployqt_post_build(test_event_bus TRUE)

add_executable(test_logger
    test_logger.cpp
)

target_link_libraries(test_logger PRIVATE
    Qt6::Test
    Qt6::Core
    soul_logging
    soul_core
)

windeployqt_post_build(test_logger TRUE)

add_executable(test_storage
    test_storage.cpp
)

target_link_libraries(test_storage PRIVATE
    Qt6::Test
    Qt6::Core
    soul_storage
    soul_core
)

windeployqt_post_build(test_storage TRUE)

add_executable(test_http_request
    test_http_request.cpp
)

target_link_libraries(test_http_request PRIVATE
    Qt6::Test
    Qt6::Core
    soul_network
    soul_core
)

windeployqt_post_build(test_http_request TRUE)

add_executable(test_network
    test_network.cpp
)

target_link_libraries(test_network PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_network
    soul_core
    soul_logging
)

windeployqt_post_build(test_network TRUE)

add_executable(test_typed_event_bus
    test_typed_event_bus.cpp
)

target_link_libraries(test_typed_event_bus PRIVATE
    Qt6::Test
    Qt6::Core
    soul_event
    soul_core
    soul_async
    soul_logging
)

windeployqt_post_build(test_typed_event_bus TRUE)

add_executable(test_cache
    test_cache.cpp
)

target_link_libraries(test_cache PRIVATE
    Qt6::Test
    Qt6::Core
    soul_storage
    soul_core
)

windeployqt_post_build(test_cache TRUE)

add_executable(test_soul_cache
    test_soul_cache.cpp
)

target_link_libraries(test_soul_cache PRIVATE
    Qt6::Test
    Qt6::Core
    soul_cache
    soul_core
)

windeployqt_post_build(test_soul_cache TRUE)

add_executable(test_async
    test_async.cpp
)

target_link_libraries(test_async PRIVATE
    Qt6::Test
    Qt6::Core
    soul_async
    soul_core
)

windeployqt_post_build(test_async TRUE)

add_executable(test_auth
    test_auth.cpp
)

target_link_libraries(test_auth PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    soul_auth
    soul_network
    soul_storage
    soul_utils
    soul_base
    soul_configuration
    soul_logging
    soul_core
)

windeployqt_post_build(test_auth TRUE)

add_executable(test_oauth2
    test_oauth2.cpp
)

target_link_libraries(test_oauth2 PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_auth
    soul_base
    soul_core
)

windeployqt_post_build(test_oauth2 TRUE)

add_executable(test_configuration
    test_configuration.cpp
)

target_link_libraries(test_configuration PRIVATE
    Qt6::Test
    Qt6::Core
    soul_configuration
    soul_core
)

windeployqt_post_build(test_configuration TRUE)

add_executable(test_core
    test_core.cpp
)

target_link_libraries(test_core PRIVATE
    Qt6::Test
    Qt6::Core
    soul_core
)

windeployqt_post_build(test_core TRUE)

add_executable(test_utils
    test_utils.cpp
)

target_link_libraries(test_utils PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Qt6::Xml
    soul_utils
    soul_core
)

windeployqt_post_build(test_utils TRUE)

add_executable(test_base
    test_base.cpp
)

target_link_libraries(test_base PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    soul_base
    soul_auth
    soul_network
    soul_storage
    soul_utils
    soul_configuration
    soul_logging
    soul_core
    soul_data
    soul_ui
)

if(TARGET Qt6::Widgets)
    target_link_libraries(test_base PRIVATE Qt6::Widgets)
endif()

windeployqt_post_build(test_base TRUE)

add_executable(test_di
    test_di.cpp
)

target_link_libraries(test_di PRIVATE
    Qt6::Test
    Qt6::Core
    soul_di
    soul_core
)

windeployqt_post_build(test_di TRUE)

add_executable(test_plugin
    test_plugin.cpp
)

target_link_libraries(test_plugin PRIVATE
    Qt6::Test
    Qt6::Core
    soul_plugin
    soul_di
    soul_core
)

windeployqt_post_build(test_plugin TRUE)

add_executable(test_data
    test_data.cpp
)

target_link_libraries(test_data PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Sql
    soul_data
    soul_core
)

windeployqt_post_build(test_data TRUE)

add_test(NAME test_configuration COMMAND test_configuration)

# v1.9.1: 配置绑定测试
add_executable(test_config_bind
    test_config_bind.cpp
)

target_link_libraries(test_config_bind PRIVATE
    Qt6::Test
    Qt6::Core
    soul_configuration
    soul_core
)

windeployqt_post_build(test_config_bind TRUE)

add_test(NAME test_config_bind COMMAND test_config_bind)
add_test(NAME test_core COMMAND test_core)

# v1.9.1: 远程配置测试 (GAP-14 Distributed Configuration Center)
add_executable(test_remote_config
    test_remote_config.cpp
)

target_link_libraries(test_remote_config PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_configuration
    soul_core
)

windeployqt_post_build(test_remote_config TRUE)

add_test(NAME test_remote_config COMMAND test_remote_config)

# v1.9.1: 模块注册表测试
add_executable(test_module_registry
    test_module_registry.cpp
)

target_link_libraries(test_module_registry PRIVATE
    Qt6::Test
    Qt6::Core
    soul_core
    soul_logging
)

windeployqt_post_build(test_module_registry TRUE)

add_test(NAME test_module_registry COMMAND test_module_registry)
add_test(NAME test_utils COMMAND test_utils)
add_test(NAME test_base COMMAND test_base)
add_test(NAME test_di COMMAND test_di)
add_test(NAME test_plugin COMMAND test_plugin)

add_test(NAME test_result COMMAND test_result)
add_test(NAME test_event_bus COMMAND test_event_bus)
add_test(NAME test_logger COMMAND test_logger)
add_test(NAME test_storage COMMAND test_storage)
add_test(NAME test_http_request COMMAND test_http_request)
add_test(NAME test_network COMMAND test_network)
add_test(NAME test_typed_event_bus COMMAND test_typed_event_bus)
add_test(NAME test_cache COMMAND test_cache)
add_test(NAME test_soul_cache COMMAND test_soul_cache)
add_test(NAME test_async COMMAND test_async)
add_test(NAME test_auth COMMAND test_auth)
add_test(NAME test_oauth2 COMMAND test_oauth2)
add_test(NAME test_data COMMAND test_data)

add_executable(test_orm
    test_orm.cpp
)

target_link_libraries(test_orm PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Sql
    soul_orm
    soul_data
    soul_core
    soul_logging
)

windeployqt_post_build(test_orm TRUE)

add_executable(test_mq
    test_mq.cpp
)

target_link_libraries(test_mq PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_mq
    soul_core
    soul_logging
    soul_async
)

windeployqt_post_build(test_mq TRUE)

add_executable(test_ui
    test_ui.cpp
)

target_link_libraries(test_ui PRIVATE
    Qt6::Test
    Qt6::Core
    soul_ui
    soul_core
    soul_base
    soul_configuration
)

if(TARGET Qt6::Widgets)
    target_link_libraries(test_ui PRIVATE Qt6::Widgets)
endif()

windeployqt_post_build(test_ui TRUE)

add_test(NAME test_orm COMMAND test_orm)
add_test(NAME test_mq COMMAND test_mq)
add_test(NAME test_ui COMMAND test_ui)

# GAP-06: Split UI component tests
if(TARGET Qt6::Widgets)
    add_executable(test_ui_button
        test_ui_button.cpp
    )

    target_link_libraries(test_ui_button PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Core
        soul_core
        soul_ui
        soul_logging
    )

    windeployqt_post_build(test_ui_button TRUE)

    add_test(NAME test_ui_button COMMAND test_ui_button)
endif()

add_executable(test_ui_switch
    test_ui_switch.cpp
)

target_link_libraries(test_ui_switch PRIVATE
    Qt6::Test
    Qt6::Core
    soul_ui
    soul_core
    soul_base
    soul_configuration
)

if(TARGET Qt6::Widgets)
    target_link_libraries(test_ui_switch PRIVATE Qt6::Widgets)
endif()

windeployqt_post_build(test_ui_switch TRUE)

add_executable(test_ui_avatar
    test_ui_avatar.cpp
)

target_link_libraries(test_ui_avatar PRIVATE
    Qt6::Test
    Qt6::Core
    soul_ui
    soul_core
    soul_base
    soul_configuration
)

if(TARGET Qt6::Widgets)
    target_link_libraries(test_ui_avatar PRIVATE Qt6::Widgets)
endif()

windeployqt_post_build(test_ui_avatar TRUE)

add_executable(test_ui_dropdown
    test_ui_dropdown.cpp
)

target_link_libraries(test_ui_dropdown PRIVATE
    Qt6::Test
    Qt6::Core
    soul_ui
    soul_core
    soul_base
    soul_configuration
)

if(TARGET Qt6::Widgets)
    target_link_libraries(test_ui_dropdown PRIVATE Qt6::Widgets)
endif()

windeployqt_post_build(test_ui_dropdown TRUE)

add_executable(test_ui_window
    test_ui_window.cpp
)

target_link_libraries(test_ui_window PRIVATE
    Qt6::Test
    Qt6::Core
    soul_ui
    soul_core
    soul_base
    soul_configuration
)

if(TARGET Qt6::Widgets)
    target_link_libraries(test_ui_window PRIVATE Qt6::Widgets)
endif()

windeployqt_post_build(test_ui_window TRUE)

add_test(NAME test_ui_switch COMMAND test_ui_switch)
add_test(NAME test_ui_avatar COMMAND test_ui_avatar)
add_test(NAME test_ui_dropdown COMMAND test_ui_dropdown)
add_test(NAME test_ui_window COMMAND test_ui_window)

add_executable(test_rpc
    test_rpc.cpp
)

target_link_libraries(test_rpc PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_rpc
    soul_core
)

windeployqt_post_build(test_rpc TRUE)

add_test(NAME test_rpc COMMAND test_rpc)

add_executable(test_observability
    test_observability.cpp
)

target_link_libraries(test_observability PRIVATE
    Qt6::Test
    Qt6::Core
    soul_observability
    soul_logging
    soul_core
)

windeployqt_post_build(test_observability TRUE)

add_test(NAME test_observability COMMAND test_observability)

# v1.9.0: 资源池监控测试
add_executable(test_resource_pool_monitor
    test_resource_pool_monitor.cpp
)

target_link_libraries(test_resource_pool_monitor PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    soul_observability
    soul_async
    soul_network
    soul_data
    soul_core
    soul_logging
)

windeployqt_post_build(test_resource_pool_monitor TRUE)

add_test(NAME test_resource_pool_monitor COMMAND test_resource_pool_monitor)

# v1.9.0: AOP 切面编程测试
add_executable(test_aop
    test_aop.cpp
)

target_link_libraries(test_aop PRIVATE
    Qt6::Test
    Qt6::Core
    soul_aop
    soul_core
    soul_logging
)

windeployqt_post_build(test_aop TRUE)

add_test(NAME test_aop COMMAND test_aop)

# v1.9.0: 嵌入式 HTTP Server 测试
add_executable(test_http_server
    test_http_server.cpp
)

target_link_libraries(test_http_server PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_server
    soul_core
    soul_logging
)

windeployqt_post_build(test_http_server TRUE)

add_test(NAME test_http_server COMMAND test_http_server)

# v1.9.3: 健康检查端点测试
add_executable(test_health
    test_health.cpp
)

target_link_libraries(test_health PRIVATE
    Qt6::Test
    Qt6::Core
    soul_server
    soul_core
    soul_logging
)

windeployqt_post_build(test_health TRUE)

add_test(NAME test_health COMMAND test_health)

# v1.9.1: WebSocket Server 测试
add_executable(test_websocket_server
    test_websocket_server.cpp
)

target_link_libraries(test_websocket_server PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    Qt6::WebSockets
    soul_server
    soul_core
    soul_logging
)

windeployqt_post_build(test_websocket_server TRUE)

add_test(NAME test_websocket_server COMMAND test_websocket_server)

# v1.9.1: ConnectionManager 测试(断线重连/心跳保活)
add_executable(test_connection_manager
    test_connection_manager.cpp
)

target_link_libraries(test_connection_manager PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_network
    soul_event
    soul_core
    soul_logging
)

windeployqt_post_build(test_connection_manager TRUE)

add_test(NAME test_connection_manager COMMAND test_connection_manager)

# v1.9.1: 定时任务框架测试
add_executable(test_scheduler
    test_scheduler.cpp
)

target_link_libraries(test_scheduler PRIVATE
    Qt6::Test
    Qt6::Core
    soul_scheduler
    soul_core
    soul_di
)

windeployqt_post_build(test_scheduler TRUE)

add_test(NAME test_scheduler COMMAND test_scheduler)

# v1.9.1: Repository 自动实现代理测试
add_executable(test_repository_factory
    test_repository_factory.cpp
)

target_link_libraries(test_repository_factory PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Sql
    soul_core
    soul_data
    soul_orm
)

windeployqt_post_build(test_repository_factory TRUE)

add_test(NAME test_repository_factory COMMAND test_repository_factory)

# v1.9.1: UI 组件补充测试 (GlassWidget/Animation/IconManager)
if(TARGET Qt6::Widgets)
    add_executable(test_ui_components1
        test_ui_components1.cpp
    )

    target_link_libraries(test_ui_components1 PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Core
        Qt6::Gui
        soul_core
        soul_ui
        soul_logging
    )

    windeployqt_post_build(test_ui_components1 TRUE)

    add_test(NAME test_ui_components1 COMMAND test_ui_components1)
endif()

# v1.9.1: UI 组件补充测试 (BaseDialog/BaseViewModel)
if(TARGET Qt6::Widgets)
    add_executable(test_ui_components2
        test_ui_components2.cpp
    )

    target_link_libraries(test_ui_components2 PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Core
        soul_core
        soul_ui
        soul_logging
    )

    windeployqt_post_build(test_ui_components2 TRUE)

    add_test(NAME test_ui_components2 COMMAND test_ui_components2)
endif()

# v1.9.1: UI 组件补充测试 (Checkbox/Card/Badge/Input/Progress)
if(TARGET Qt6::Widgets)
    add_executable(test_ui_components3
        test_ui_components3.cpp
    )

    target_link_libraries(test_ui_components3 PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Core
        soul_core
        soul_ui
        soul_logging
    )

    windeployqt_post_build(test_ui_components3 TRUE)

    add_test(NAME test_ui_components3 COMMAND test_ui_components3)
endif()

# v1.9.1: UI 组件补充测试 (Slider/Spinner/TabBar/Toast/ToolTip)
if(TARGET Qt6::Widgets)
    add_executable(test_ui_components4
        test_ui_components4.cpp
    )

    target_link_libraries(test_ui_components4 PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Core
        soul_core
        soul_ui
        soul_logging
    )

    windeployqt_post_build(test_ui_components4 TRUE)

    add_test(NAME test_ui_components4 COMMAND test_ui_components4)
endif()

# v1.9.1: UI 组件补充测试 (Loading/EmptyWidget/Page/SideBar/Navigation)
if(TARGET Qt6::Widgets)
    add_executable(test_ui_components5
        test_ui_components5.cpp
    )

    target_link_libraries(test_ui_components5 PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Core
        soul_core
        soul_ui
        soul_logging
    )

    windeployqt_post_build(test_ui_components5 TRUE)

    add_test(NAME test_ui_components5 COMMAND test_ui_components5)
endif()

# v1.9.1: UI 组件补充测试 (BaseWidget/BaseView/Icon/ScrollBar)
if(TARGET Qt6::Widgets)
    add_executable(test_ui_components6
        test_ui_components6.cpp
    )

    target_link_libraries(test_ui_components6 PRIVATE
        Qt6::Test
        Qt6::Widgets
        Qt6::Core
        soul_core
        soul_ui
        soul_logging
    )

    windeployqt_post_build(test_ui_components6 TRUE)

    add_test(NAME test_ui_components6 COMMAND test_ui_components6)
endif()

# v1.9.3: CircuitBreaker / RateLimiter / Validator / InfoEndpoint / LoggersEndpoint 综合测试
add_executable(test_v193_components
    test_v193_components.cpp
)

target_link_libraries(test_v193_components PRIVATE
    Qt6::Test
    Qt6::Core
    Qt6::Network
    soul_server
    soul_network
    soul_validation
    soul_observability
    soul_core
    soul_logging
)

windeployqt_post_build(test_v193_components TRUE)

add_test(NAME test_v193_components COMMAND test_v193_components)

# v1.9.1: C++20 coroutine support tests (GAP-15)
if(ENABLE_CXX20)
    add_executable(test_coroutine
        test_coroutine.cpp
    )
    target_link_libraries(test_coroutine PRIVATE
        Qt6::Test
        Qt6::Core
        soul_async
        soul_core
    )
    windeployqt_post_build(test_coroutine TRUE)
    add_test(NAME test_coroutine COMMAND test_coroutine)
endif()
