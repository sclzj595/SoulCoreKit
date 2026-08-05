# network_protocol.cmake — 网络协议层 [v2.5.0]
# 依赖: core + network_core

add_library(soul_network_protocol STATIC
    src/soul/network/bluetooth/bluetooth_client_adapter.cpp
    src/soul/network/mqtt/mqtt_client_adapter.cpp
    src/soul/network/namedpipe/named_pipe_adapter.cpp
    src/soul/network/serial/serial_port_adapter.cpp
    src/soul/network/tcp/tcp_client_adapter.cpp
    src/soul/network/websocket/ws_client_adapter.cpp
)

target_include_directories(soul_network_protocol PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_network_protocol PUBLIC soul_core soul_network_core)