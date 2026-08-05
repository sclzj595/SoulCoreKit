# server.cmake — 嵌入式 HTTP Server 模块 [v2.5.0]
# 依赖: core + logging + network + observability + scheduler + Qt::Network + Qt::WebSockets

add_library(soul_server STATIC
    src/soul/server/health.cpp
    src/soul/server/http_server.cpp
    src/soul/server/mappings_endpoint.cpp
    src/soul/server/middleware.cpp
    src/soul/server/websocket_server.cpp
)

target_include_directories(soul_server PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
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