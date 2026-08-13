# auth.cmake — 认证授权模块 [v2.5.0]
# Layer: Extensions (E03) — 企业级扩展
# 依赖: core + network + storage + utils
# 职责: AuthManager/TokenManager/Permission/OAuth2/OIDC/SecureStorage/User
#       CS/BS 双端共享认证逻辑

add_library(soul_auth STATIC
    src/soul/auth/auth_manager.cpp
    src/soul/auth/oauth2.cpp
    src/soul/auth/oidc.cpp
    src/soul/auth/permission.cpp
    src/soul/auth/token_manager.cpp
    # v3.0.0: Q_OBJECT 头文件加入 sources 以便 AUTOMOC 扫描
    include/soul/auth/auth_manager.h
    include/soul/auth/oauth2.h
    include/soul/auth/oidc.h
)

target_include_directories(soul_auth PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
)

target_link_libraries(soul_auth PUBLIC soul_core soul_network soul_storage soul_utils)