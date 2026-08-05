# event.cmake — 事件总线模块 [v2.5.0]
# 依赖: core + async (PRIVATE) + logging (PRIVATE)

add_library(soul_event STATIC
    src/soul/event/event_bus.cpp
    src/soul/event/qt_signal_adapter.cpp
    src/soul/event/subscription.cpp
    src/soul/event/typed_event_bus_detail.cpp
)

target_include_directories(soul_event PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_event PUBLIC soul_core)
target_link_libraries(soul_event PRIVATE soul_async soul_logging)