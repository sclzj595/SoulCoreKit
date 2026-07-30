#ifndef SOUL_LOGGING_LOG_MACROS_H
#define SOUL_LOGGING_LOG_MACROS_H

#include "soul/logging/logger.h"

// ============================================================================
// 基础日志宏 — 无模块分类
// ============================================================================
#define SC_TRACE(msg)    sc::Logger::instance().trace(msg)
#define SC_DEBUG(msg)    sc::Logger::instance().debug(msg)
#define SC_INFO(msg)     sc::Logger::instance().info(msg)
#define SC_WARN(msg)     sc::Logger::instance().warn(msg)
#define SC_ERROR(msg)    sc::Logger::instance().error(msg)
#define SC_FATAL(msg)    sc::Logger::instance().fatal(msg)

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
//   // 在模块头文件中声明日志分类
//   SC_LOG_CATEGORY(orm)
//
//   // 在模块代码中使用
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

#endif