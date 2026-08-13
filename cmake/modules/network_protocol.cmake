# network_protocol.cmake — 网络协议层 [v2.5.0]
# Layer: Infrastructure (I04) — 通信基础设施
# 依赖: core + network_core
# 职责: TcpClientAdapter/WebSocketAdapter/MqttAdapter/BluetoothAdapter/SerialPortAdapter/NamedPipeAdapter
#       统一抽象: TCP(CS) / HTTP(BS) / WebSocket(CS/BS)

add_library(soul_network_protocol STATIC
    src/soul/network/bluetooth/bluetooth_client_adapter.cpp
    src/soul/network/mqtt/mqtt_client_adapter.cpp
    src/soul/network/namedpipe/named_pipe_adapter.cpp
    src/soul/network/serial/serial_port_adapter.cpp
    src/soul/network/tcp/tcp_client_adapter.cpp
    src/soul/network/websocket/ws_client_adapter.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/network/tcp/tcp_client_adapter.h
    include/soul/network/websocket/ws_client_adapter.h
    include/soul/network/mqtt/mqtt_client_adapter.h
    include/soul/network/serial/serial_port_adapter.h
    include/soul/network/namedpipe/named_pipe_adapter.h
    include/soul/network/bluetooth/bluetooth_client_adapter.h
)

target_include_directories(soul_network_protocol PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(soul_network_protocol PUBLIC SC_NETWORK_STATIC_LIB)
target_link_libraries(soul_network_protocol PUBLIC soul_core soul_network_core Qt6::Network Qt6::WebSockets)