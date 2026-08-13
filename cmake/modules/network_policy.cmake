# network_policy.cmake — 网络策略层 [v2.5.0]
# Layer: Infrastructure (I02) — 通信基础设施
# 依赖: core + network_core
# 职责: CircuitBreaker/RetryPolicy/RateLimiter/HeartbeatPolicy/ReconnectPolicy/TimeoutPolicy

add_library(soul_network_policy STATIC
    src/soul/network/policy/circuit_breaker.cpp
    src/soul/network/policy/heartbeat_policy.cpp
    src/soul/network/policy/rate_limiter.cpp
    src/soul/network/policy/reconnect_policy.cpp
    src/soul/network/policy/retry_policy.cpp
    src/soul/network/policy/timeout_policy.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/network/policy/heartbeat_policy.h
)

target_include_directories(soul_network_policy PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_compile_definitions(soul_network_policy PUBLIC SC_NETWORK_STATIC_LIB)
target_link_libraries(soul_network_policy PUBLIC soul_core soul_network_core)