# server.cmake — 嵌入式 HTTP/WebSocket Server 模块 [v2.5.0]
# Layer: Extensions (E06) — 企业级扩展 (BS 场景核心入口)
# 依赖: core + logging + network + observability + scheduler + Qt::Network + Qt::WebSockets
# 职责: HttpServer(自研轻量HTTP/1.1)/WebSocketServer/Middleware/Health/MappingsEndpoint
#       BS Web Backend / REST API 场景核心依赖

add_library(soul_server STATIC
    src/soul/server/health.cpp
    src/soul/server/http_server.cpp
    src/soul/server/mappings_endpoint.cpp
    src/soul/server/middleware.cpp
    src/soul/server/websocket_server.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/server/http_server.h
    include/soul/server/websocket_server.h
)

target_include_directories(soul_server PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_server PUBLIC
    soul_core
    soul_logging
    soul_network
    soul_observability
    soul_scheduler
    Qt6::Network
    Qt6::WebSockets
)