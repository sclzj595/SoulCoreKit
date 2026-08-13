# network_http.cmake — 网络 HTTP 层 [v2.5.0]
# Layer: Infrastructure (I03) — 通信基础设施
# 依赖: core + network_core + network_policy
# 职责: HttpClientAdapter/AuthInterceptor/LoggingInterceptor/拦截器链

add_library(soul_network_http STATIC
    src/soul/network/http/http_client_adapter.cpp
    src/soul/network/interceptor/auth_interceptor.cpp
    src/soul/network/interceptor/logging_interceptor.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/network/http/http_client_adapter.h
)

target_include_directories(soul_network_http PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(soul_network_http PUBLIC SC_NETWORK_STATIC_LIB)
target_link_libraries(soul_network_http PUBLIC soul_core soul_network_core soul_network_policy Qt6::Network)