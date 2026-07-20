find_package(Qt6 REQUIRED COMPONENTS Test Core Network)

add_executable(test_result
    test_result.cpp
)

target_link_libraries(test_result PRIVATE
    Qt6::Test
    Qt6::Core
    soul_core
)

copy_qt_dll(test_result TRUE)

add_executable(test_event_bus
    test_event_bus.cpp
)

target_link_libraries(test_event_bus PRIVATE
    Qt6::Test
    Qt6::Core
    soul_event
    soul_core
)

copy_qt_dll(test_event_bus TRUE)

add_executable(test_logger
    test_logger.cpp
)

target_link_libraries(test_logger PRIVATE
    Qt6::Test
    Qt6::Core
    soul_logging
    soul_core
)

copy_qt_dll(test_logger TRUE)

add_executable(test_storage
    test_storage.cpp
)

target_link_libraries(test_storage PRIVATE
    Qt6::Test
    Qt6::Core
    soul_storage
    soul_core
)

copy_qt_dll(test_storage TRUE)

add_executable(test_http_request
    test_http_request.cpp
)

target_link_libraries(test_http_request PRIVATE
    Qt6::Test
    Qt6::Core
    soul_network
    soul_core
)

copy_qt_dll(test_http_request TRUE)

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

copy_qt_dll(test_network TRUE)

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

copy_qt_dll(test_typed_event_bus TRUE)

add_executable(test_cache
    test_cache.cpp
)

target_link_libraries(test_cache PRIVATE
    Qt6::Test
    Qt6::Core
    soul_storage
    soul_core
)

copy_qt_dll(test_cache TRUE)

add_executable(test_async
    test_async.cpp
)

target_link_libraries(test_async PRIVATE
    Qt6::Test
    Qt6::Core
    soul_async
    soul_core
)

copy_qt_dll(test_async TRUE)

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

copy_qt_dll(test_auth TRUE)

add_executable(test_configuration
    test_configuration.cpp
)

target_link_libraries(test_configuration PRIVATE
    Qt6::Test
    Qt6::Core
    soul_configuration
    soul_core
)

copy_qt_dll(test_configuration TRUE)

add_executable(test_core
    test_core.cpp
)

target_link_libraries(test_core PRIVATE
    Qt6::Test
    Qt6::Core
    soul_core
)

copy_qt_dll(test_core TRUE)

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

copy_qt_dll(test_utils TRUE)

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

copy_qt_dll(test_base TRUE)

add_executable(test_di
    test_di.cpp
)

target_link_libraries(test_di PRIVATE
    Qt6::Test
    Qt6::Core
    soul_di
    soul_core
)

copy_qt_dll(test_di TRUE)

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

copy_qt_dll(test_plugin TRUE)

add_test(NAME test_configuration COMMAND test_configuration)
add_test(NAME test_core COMMAND test_core)
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
add_test(NAME test_async COMMAND test_async)
add_test(NAME test_auth COMMAND test_auth)
