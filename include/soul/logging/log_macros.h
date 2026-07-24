#ifndef SOUL_LOGGING_LOG_MACROS_H
#define SOUL_LOGGING_LOG_MACROS_H

#include "soul/logging/logger.h"

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

#endif