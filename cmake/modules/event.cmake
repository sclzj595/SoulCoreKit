# event.cmake — 事件总线模块 [v2.9.1]
# Layer: Core (C05) — CS/BS 共用极稳定核心
# 依赖: core + async (PRIVATE) + logging (PRIVATE)
# 职责: EventBus/TypedEventBus/IMessageBus/InMemoryMessageBus/Subscription/QtSignalAdapter

add_library(soul_event STATIC
    src/soul/event/event_bus.cpp
    src/soul/event/qt_signal_adapter.cpp
    src/soul/event/subscription.cpp
    src/soul/event/typed_event_bus_detail.cpp
    src/soul/event/inmemory_message_bus.cpp  # v2.9.1
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/event/qt_signal_adapter.h
)

target_include_directories(soul_event PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_event PUBLIC soul_core)
target_link_libraries(soul_event PRIVATE soul_async soul_logging)