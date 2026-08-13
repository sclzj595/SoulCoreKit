#ifndef SOUL_LOGGING_LOG_MACROS_H
#define SOUL_LOGGING_LOG_MACROS_H

#include "soul/logging/logger.h"
#include "soul/core/request_context.h"  // v2.9.0: Context 自动关联

// ============================================================================
// v2.9.0: Context 感知格式化辅助
// ============================================================================
// 当 RequestContextGuard 存在时，自动附加 requestId / traceId
#define SC_FORMAT_WITH_CTX(msg) \
    (sc::RequestContextGuard::hasCurrent() \
        ? (std::string("[") + \
           sc::RequestContextGuard::currentRequestId().left(8).toStdString() + ":" + \
           sc::RequestContextGuard::currentTraceId().left(8).toStdString() + "] " + \
           std::string(msg)) \
        : std::string(msg))

// ============================================================================
// 基础日志宏 — 无模块分类 (兼容旧版)
// ============================================================================
#define SC_TRACE(msg)    sc::Logger::instance().trace(SC_FORMAT_WITH_CTX(msg))
#define SC_DEBUG(msg)    sc::Logger::instance().debug(SC_FORMAT_WITH_CTX(msg))
#define SC_INFO(msg)     sc::Logger::instance().info(SC_FORMAT_WITH_CTX(msg))
#define SC_WARN(msg)     sc::Logger::instance().warn(SC_FORMAT_WITH_CTX(msg))
#define SC_ERROR(msg)    sc::Logger::instance().error(SC_FORMAT_WITH_CTX(msg))
#define SC_FATAL(msg)    sc::Logger::instance().fatal(SC_FORMAT_WITH_CTX(msg))

#define SC_TRACE_S(module, op, msg)    sc::Logger::instance().trace(msg, module, op)
#define SC_DEBUG_S(module, op, msg)    sc::Logger::instance().debug(msg, module, op)
#define SC_INFO_S(module, op, msg)     sc::Logger::instance().info(msg, module, op)
#define SC_WARN_S(module, op, msg)     sc::Logger::instance().warn(msg, module, op)
#define SC_ERROR_S(module, op, msg)    sc::Logger::instance().error(msg, module, op)
#define SC_FATAL_S(module, op, msg)    sc::Logger::instance().fatal(msg, module, op)

// ============================================================================
// 模块级日志宏 [v1.9.2 新增]
// ============================================================================
//
// 用法:
//   // 在模块中使用
//   SC_LOGC_TRACE(orm, "Starting migration...")
//   SC_LOGC_DEBUG(orm, "Query executed in 3ms")
//   SC_LOGC_INFO(orm, "Migration completed")
//   SC_LOGC_WARN(orm, "Deprecated field found")
//   SC_LOGC_ERROR(orm, "Migration failed")
//   SC_LOGC_FATAL(orm, "Database corrupted")
//
// 模块级日志过滤:
//   sc::Logger::instance().setModuleLogLevel("orm", LogLevel::Warn);
//   // 仅 WARN 及以上级别的 orm 日志会被输出

/// @brief 声明模块日志分类(在头文件中使用)
#define SC_LOG_CATEGORY(name) \
    static constexpr const char* _sc_log_category = #name

/// @brief 带模块分类的日志宏
#define SC_LOGC_TRACE(cat, msg)    sc::Logger::instance().trace(msg, #cat)
#define SC_LOGC_DEBUG(cat, msg)    sc::Logger::instance().debug(msg, #cat)
#define SC_LOGC_INFO(cat, msg)     sc::Logger::instance().info(msg, #cat)
#define SC_LOGC_WARN(cat, msg)     sc::Logger::instance().warn(msg, #cat)
#define SC_LOGC_ERROR(cat, msg)    sc::Logger::instance().error(msg, #cat)
#define SC_LOGC_FATAL(cat, msg)    sc::Logger::instance().fatal(msg, #cat)

/// @brief 带模块分类和操作的日志宏
#define SC_LOGC_TRACE_S(cat, op, msg)    sc::Logger::instance().trace(msg, #cat, op)
#define SC_LOGC_DEBUG_S(cat, op, msg)    sc::Logger::instance().debug(msg, #cat, op)
#define SC_LOGC_INFO_S(cat, op, msg)     sc::Logger::instance().info(msg, #cat, op)
#define SC_LOGC_WARN_S(cat, op, msg)     sc::Logger::instance().warn(msg, #cat, op)
#define SC_LOGC_ERROR_S(cat, op, msg)    sc::Logger::instance().error(msg, #cat, op)
#define SC_LOGC_FATAL_S(cat, op, msg)    sc::Logger::instance().fatal(msg, #cat, op)

// ============================================================================
// fmt 风格格式化日志宏 [v1.9.3 新增]
// ============================================================================
//
// 用法:
//   SC_TRACE_FMT("Processing item {} of {}", i, total);
//   SC_DEBUG_FMT("User {} logged in from {}", username, ip);
//   SC_INFO_FMT("Server started on port {}", port);
//   SC_WARN_FMT("Connection timeout after {}ms", timeout);
//   SC_ERROR_FMT("Failed to open file: {}", filePath);
//   SC_FATAL_FMT("Out of memory, requested {} bytes", size);
//
// 说明:
//   - 使用 spdlog/fmt 风格格式化,性能优于字符串拼接
//   - 支持所有 fmt 格式说明符 ({} , {:.2f}, {:x} 等)
//   - 类型安全,编译期检查格式字符串

#define SC_TRACE_FMT(...)    sc::Logger::instance().traceFmt(__VA_ARGS__)
#define SC_DEBUG_FMT(...)    sc::Logger::instance().debugFmt(__VA_ARGS__)
#define SC_INFO_FMT(...)     sc::Logger::instance().infoFmt(__VA_ARGS__)
#define SC_WARN_FMT(...)     sc::Logger::instance().warnFmt(__VA_ARGS__)
#define SC_ERROR_FMT(...)    sc::Logger::instance().errorFmt(__VA_ARGS__)
#define SC_FATAL_FMT(...)    sc::Logger::instance().fatalFmt(__VA_ARGS__)

// ============================================================================
// SPDLOG 原生宏透传 [v1.9.3 新增]
// ============================================================================
//
// 用法:
//   SC_LOG_TRACE("spdlog native: {}", value);
//   SC_LOG_DEBUG("spdlog native: {}", value);
//   SC_LOG_INFO("spdlog native: {}", value);
//   SC_LOG_WARN("spdlog native: {}", value);
//   SC_LOG_ERROR("spdlog native: {}", value);
//   SC_LOG_CRITICAL("spdlog native: {}", value);
//
// 说明:
//   - 与 SC_*_FMT 宏实质相同, 提供 spdlog 风格命名别名
//   - 使用 spdlog::format_string_t 编译期格式检查
//   - @note 不包含文件名/行号/函数名, 需要源位置信息请使用 spdlog 原生宏
//     (如 SPDLOG_INFO) 或自行在日志消息中拼接 __FILE__/__LINE__
//
// Thread-safety: via *Fmt()->checkAndLog() (holds m_mutex;
// excludes add*Sink). Configure sinks at init, before logging.
#define SC_LOG_TRACE(...)    sc::Logger::instance().traceFmt(__VA_ARGS__)
#define SC_LOG_DEBUG(...)    sc::Logger::instance().debugFmt(__VA_ARGS__)
#define SC_LOG_INFO(...)     sc::Logger::instance().infoFmt(__VA_ARGS__)
#define SC_LOG_WARN(...)     sc::Logger::instance().warnFmt(__VA_ARGS__)
#define SC_LOG_ERROR(...)    sc::Logger::instance().errorFmt(__VA_ARGS__)
#define SC_LOG_CRITICAL(...) sc::Logger::instance().fatalFmt(__VA_ARGS__)

#endif