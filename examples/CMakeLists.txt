# ============================================================================
# SoulCoreKit Examples
# ============================================================================
#
# 示例分类:
#   - skeleton_main:    最小骨架示例(推荐脚手架用户首先阅读)
#   - logger_example:   单一关注点示例 - 日志
#   - network_example:  单一关注点示例 - HTTP
#   - simple_test:      极简示例 - UUID
#   - soulcore_demo:    UI 演示(需要 Widgets)
#   - full_stack_example: 完整集成示例(可选,默认关闭,违背脚手架轻量定位)

# 添加 BUILD_FULL_STACK_EXAMPLE 开关
# 默认 OFF: full_stack_example 演示 ORM+MQ+DB 完整堆栈,与脚手架"够用即可"原则不符。
# 用户如需查看完整集成演示,显式开启: cmake -DBUILD_FULL_STACK_EXAMPLE=ON ..
option(BUILD_FULL_STACK_EXAMPLE "Build full-stack integration example (ORM+MQ+DB)" OFF)

# --- 最小骨架示例(脚手架首选示例) ---
add_executable(skeleton_example skeleton_main.cpp)
target_link_libraries(skeleton_example PRIVATE SoulCoreKit)
windeployqt_post_build(skeleton_example FALSE)

# --- 单一关注点示例 ---
add_executable(logger_example logger_example.cpp)
target_link_libraries(logger_example PRIVATE SoulCoreKit)
windeployqt_post_build(logger_example FALSE)

add_executable(network_example network_example.cpp)
target_link_libraries(network_example PRIVATE SoulCoreKit)
windeployqt_post_build(network_example FALSE)

add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE soul_core)

# --- UI 演示(需要 Widgets) ---
add_executable(soulcore_demo main.cpp)
target_link_libraries(soulcore_demo PRIVATE SoulCoreKit SoulCoreKitUi)
windeployqt_post_build(soulcore_demo FALSE)

# --- 完整集成示例(可选,默认 OFF) ---
if(BUILD_FULL_STACK_EXAMPLE)
    add_executable(full_stack_example full_stack_example.cpp)
    target_link_libraries(full_stack_example PRIVATE SoulCoreKit)
    windeployqt_post_build(full_stack_example FALSE)
endif()
