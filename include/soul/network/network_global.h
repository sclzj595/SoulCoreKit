/**
 * @file network_global.h
 * @brief 网络模块全局导出宏定义
 * @details 定义 SC_NETWORK_EXPORT 宏，用于 DLL 导出/导入控制
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef SC_NETWORK_GLOBAL_H
#define SC_NETWORK_GLOBAL_H

#include <QtGlobal>

#if defined(SC_NETWORK_STATIC_LIB)
#  define SC_NETWORK_EXPORT
#elif defined(SC_NETWORK_LIBRARY)
#  define SC_NETWORK_EXPORT Q_DECL_EXPORT
#else
#  define SC_NETWORK_EXPORT Q_DECL_IMPORT
#endif

#endif