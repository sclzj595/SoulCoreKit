# network.cmake — 网络通信聚合模块 [v2.5.0]
# Layer: Infrastructure (I05) — 通信基础设施
# 依赖: core + network_core + network_policy + network_http + network_protocol + Qt::Network + Qt::WebSockets
# 职责: HttpClient/WebSocket/TcpClient/Downloader/Uploader/ConnectionManager/ConnectionPool/Session/CookieJar
#       SoulCoreKit 核心竞争力 — CS/BS 统一通信抽象

add_library(soul_network STATIC
    src/soul/network/connection_manager.cpp
    src/soul/network/cookie_jar.cpp
    src/soul/network/downloader.cpp
    src/soul/network/http_client.cpp
    src/soul/network/http_request.cpp
    src/soul/network/http_response.cpp
    # v3.0.0: network_error.cpp 已移至 soul_network_core（避免循环依赖）
    src/soul/network/session.cpp
    src/soul/network/tcp_client.cpp
    src/soul/network/uploader.cpp
    src/soul/network/web_socket.cpp
    src/soul/network/factory/network_factory.cpp
    src/soul/network/pool/connection_pool.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/network/connection_manager.h
    include/soul/network/cookie_jar.h
    include/soul/network/http_client.h
    include/soul/network/downloader.h
    include/soul/network/uploader.h
    include/soul/network/tcp_client.h
    include/soul/network/web_socket.h
    include/soul/network/pool/connection_pool.h
)

target_include_directories(soul_network PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(soul_network PUBLIC SC_NETWORK_STATIC_LIB)
target_link_libraries(soul_network PUBLIC
    soul_core
    soul_network_core
    soul_network_policy
    soul_network_http
    soul_network_protocol
    Qt6::Network
    Qt6::WebSockets
    nlohmann_json::nlohmann_json
)