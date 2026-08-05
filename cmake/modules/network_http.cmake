# network_http.cmake — 网络 HTTP 层 [v2.5.0]
# 依赖: core + network_core + network_policy

add_library(soul_network_http STATIC
    src/soul/network/http/http_client_adapter.cpp
    src/soul/network/interceptor/auth_interceptor.cpp
    src/soul/network/interceptor/logging_interceptor.cpp
)

target_include_directories(soul_network_http PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_network_http PUBLIC soul_core soul_network_core soul_network_policy)