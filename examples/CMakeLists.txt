# ============================================================================
# SoulCoreKit Examples — CS+BS 双场景示例
# ============================================================================
#
# 聚合库选择指南:
#   SoulCoreKit        — BS Backend / CLI 工具 / Headless 服务 (不含 UI)
#   SoulCoreKitUi      — SoulCoreKit + UI 组件库 (CS Client)
#   SoulCoreKitFull    — 全量聚合含 CS 架构 (完整 CS 应用)
#
# 示例分类:
#   - skeleton_main:       最小骨架示例 (BS/CLI 场景推荐)
#   - logger_example:      日志系统示例
#   - network_example:     HTTP Client 示例
#   - simple_test:         极简 Core 示例 (仅 soul_core)
#   - soulcore_demo:       UI 演示 (CS Client 场景, 需要 Widgets)
#   - full_stack_example:  全栈集成示例 (可选, 默认关闭)

option(BUILD_FULL_STACK_EXAMPLE "Build full-stack integration example (ORM+MQ+DB)" OFF)

# --- BS/CLI 场景: 最小骨架示例 ---
add_executable(skeleton_example skeleton_main.cpp)
target_link_libraries(skeleton_example PRIVATE SoulCoreKit)
windeployqt_post_build(skeleton_example FALSE)

# --- BS/CLI 场景: 单一关注点示例 ---
add_executable(logger_example logger_example.cpp)
target_link_libraries(logger_example PRIVATE SoulCoreKit)
windeployqt_post_build(logger_example FALSE)

add_executable(network_example network_example.cpp)
target_link_libraries(network_example PRIVATE SoulCoreKit)
windeployqt_post_build(network_example FALSE)

# --- 极简 Core 示例 (仅 soul_core) ---
add_executable(simple_test simple_test.cpp)
target_link_libraries(simple_test PRIVATE soul_core)

# --- CS Client 场景: UI 演示 (需要 Widgets) ---
add_executable(soulcore_demo main.cpp)
target_link_libraries(soulcore_demo PRIVATE SoulCoreKitFull)  # CS 全量: Core + UI + CS 架构
windeployqt_post_build(soulcore_demo FALSE)

# --- BS Backend 场景: 端到端示例 [v2.7.0 新增] ---
add_executable(bs_backend bs_backend/bs_backend_main.cpp)
target_link_libraries(bs_backend PRIVATE SoulCoreKit)  # BS 场景: 不含 UI/CS
windeployqt_post_build(bs_backend FALSE)

# --- CLI / Headless 场景: 最小依赖示例 [v2.7.0 新增] ---
add_executable(cli_tool cli/cli_main.cpp)
target_link_libraries(cli_tool PRIVATE SoulCoreKit)  # CLI: 不含 UI/CS

# --- 完整集成示例 (可选, 默认 OFF) ---
if(BUILD_FULL_STACK_EXAMPLE)
    add_executable(full_stack_example full_stack_example.cpp)
    target_link_libraries(full_stack_example PRIVATE SoulCoreKitFull)
    windeployqt_post_build(full_stack_example FALSE)
endif()
