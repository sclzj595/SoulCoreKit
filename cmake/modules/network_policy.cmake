# network_policy.cmake — 网络策略层 [v2.5.0]
# 依赖: core + network_core

add_library(soul_network_policy STATIC
    src/soul/network/policy/circuit_breaker.cpp
    src/soul/network/policy/heartbeat_policy.cpp
    src/soul/network/policy/rate_limiter.cpp
    src/soul/network/policy/reconnect_policy.cpp
    src/soul/network/policy/retry_policy.cpp
    src/soul/network/policy/timeout_policy.cpp
)

target_include_directories(soul_network_policy PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_network_policy PUBLIC soul_core soul_network_core)