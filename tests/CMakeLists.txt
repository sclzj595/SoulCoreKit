find_package(Qt6 REQUIRED COMPONENTS Test Core Network)

add_executable(test_result
    test_result.cpp
)

target_link_libraries(test_result PRIVATE
    Qt6::Test
    Qt6::Core
    soul_core
)

copy_qt_dll(test_result)

add_executable(test_event_bus
    test_event_bus.cpp
)

target_link_libraries(test_event_bus PRIVATE
    Qt6::Test
    Qt6::Core
    soul_event
    soul_core
)

copy_qt_dll(test_event_bus)

add_executable(test_logger
    test_logger.cpp
)

target_link_libraries(test_logger PRIVATE
    Qt6::Test
    Qt6::Core
    soul_logging
    soul_core
)

copy_qt_dll(test_logger)

add_executable(test_storage
    test_storage.cpp
)

target_link_libraries(test_storage PRIVATE
    Qt6::Test
    Qt6::Core
    soul_storage
    soul_core
)

copy_qt_dll(test_storage)

add_executable(test_http_request
    test_http_request.cpp
)

target_link_libraries(test_http_request PRIVATE
    Qt6::Test
    Qt6::Core
    soul_network
    soul_core
)

copy_qt_dll(test_http_request)

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

copy_qt_dll(test_network)

add_test(NAME test_result COMMAND test_result)
add_test(NAME test_event_bus COMMAND test_event_bus)
add_test(NAME test_logger COMMAND test_logger)
add_test(NAME test_storage COMMAND test_storage)
add_test(NAME test_http_request COMMAND test_http_request)
add_test(NAME test_network COMMAND test_network)
