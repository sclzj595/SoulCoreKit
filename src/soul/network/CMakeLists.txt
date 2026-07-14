add_library(soul_network STATIC
    core/inetwork.h
    core/network_base.h
    core/network_adapter_base.h
    core/network_adapter_base.cpp
    core/network_message.h
    core/network_state.h
    policy/inetwork_policy.h
    policy/timeout_policy.h
    policy/timeout_policy.cpp
    policy/retry_policy.h
    policy/retry_policy.cpp
    interceptor/i_interceptor.h
    interceptor/logging_interceptor.h
    interceptor/logging_interceptor.cpp
    interceptor/auth_interceptor.h
    interceptor/auth_interceptor.cpp
    factory/network_factory.h
    http/http_client_adapter.h
    http/http_client_adapter.cpp
    tcp/tcp_client_adapter.h
    tcp/tcp_client_adapter.cpp
    websocket/ws_client_adapter.h
    websocket/ws_client_adapter.cpp
)

target_include_directories(soul_network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
    ${CMAKE_CURRENT_BINARY_DIR}/../..
)

target_link_libraries(soul_network PUBLIC
    soul_core
    soul_logging
)