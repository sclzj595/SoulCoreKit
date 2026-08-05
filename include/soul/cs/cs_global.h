#ifndef SOUL_CS_GLOBAL_H
#define SOUL_CS_GLOBAL_H

// ============================================================================
// cs_global.h — CS 架构核心模块导出宏 [v2.5.0]
// ============================================================================
//
// 对标 SpringBoot 的 spring-boot-starter-web，提供 CS 场景的
// Controller/Service/ViewModel/Router 核心抽象。
//
// 关系: sc::cs 包装 sc::ui 的现有组件，增加 SpringBoot 风格抽象层。
//
// 导出宏说明:
//   - 当前 soul_cs 始终构建为静态库，SC_CS_EXPORT 为空
//   - 与 soul_ui 模块保持一致（无导出宏）
//   - 未来若需动态库支持，可按 SOUL_CS_LIBRARY / SOUL_CS_STATIC_LIB 条件展开
// ============================================================================

#include <QtCore/QtGlobal>

// 静态库：导出宏始终为空
#define SC_CS_EXPORT

namespace sc::cs {

/// @brief CS 模块版本信息
struct CsVersion {
    static constexpr int Major = 2;
    static constexpr int Minor = 5;
    static constexpr int Patch = 0;
    static constexpr const char* String = "2.5.0";
};

} // namespace sc::cs

#endif // SOUL_CS_GLOBAL_H