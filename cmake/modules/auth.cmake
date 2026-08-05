# auth.cmake — 认证模块 [v2.5.0]
# 依赖: core + network + storage + utils

add_library(soul_auth STATIC
    src/soul/auth/auth_manager.cpp
    src/soul/auth/oauth2.cpp
    src/soul/auth/oidc.cpp
    src/soul/auth/permission.cpp
    src/soul/auth/token_manager.cpp
)

target_include_directories(soul_auth PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
)

target_link_libraries(soul_auth PUBLIC soul_core soul_network soul_storage soul_utils)