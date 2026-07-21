add_executable(logger_example logger_example.cpp)
target_link_libraries(logger_example PRIVATE SoulCoreKit)
copy_qt_dll(logger_example FALSE)

add_executable(network_example network_example.cpp)
target_link_libraries(network_example PRIVATE SoulCoreKit)
copy_qt_dll(network_example FALSE)

add_executable(soulcore_demo main.cpp)
target_link_libraries(soulcore_demo PRIVATE SoulCoreKit)
copy_qt_dll(soulcore_demo FALSE)

add_executable(full_stack_example full_stack_example.cpp)
target_link_libraries(full_stack_example PRIVATE SoulCoreKit)
copy_qt_dll(full_stack_example FALSE)

add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE soul_core)
copy_qt_dll(simple_test FALSE)