# network.cmake — 网络通信聚合模块 [v2.5.0]
# 依赖: core + network_core + network_policy + network_http + network_protocol + Qt::Network + Qt::WebSockets

add_library(soul_network STATIC
    src/soul/network/connection_manager.cpp
    src/soul/network/cookie_jar.cpp
    src/soul/network/downloader.cpp
    src/soul/network/http_client.cpp
    src/soul/network/http_request.cpp
    src/soul/network/http_response.cpp
    src/soul/network/network_error.cpp
    src/soul/network/session.cpp
    src/soul/network/tcp_client.cpp
    src/soul/network/uploader.cpp
    src/soul/network/web_socket.cpp
    src/soul/network/factory/network_factory.cpp
    src/soul/network/pool/connection_pool.cpp
)

target_include_directories(soul_network PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_network PUBLIC
    soul_core
    soul_network_core
    soul_network_policy
    soul_network_http
    soul_network_protocol
    Qt6::Network
    Qt6::WebSockets
)