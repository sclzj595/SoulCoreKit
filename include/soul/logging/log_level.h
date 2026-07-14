#ifndef SOUL_LOGGING_LOG_LEVEL_H
#define SOUL_LOGGING_LOG_LEVEL_H

namespace sc {

/**
 * @enum LogLevel
 * @brief 日志级别枚举
 *
 * 日志级别从低到高依次为：Trace < Debug < Info < Warn < Error < Fatal
 * 高优先级的日志会包含低优先级的日志（取决于日志配置）。
 */
enum class LogLevel {
    Trace = 0,  ///< 最详细的日志级别，用于追踪程序执行流程
    Debug = 1,  ///< 调试信息，用于开发阶段调试
    Info  = 2,  ///< 普通信息，记录程序运行的关键节点
    Warn  = 3,  ///< 警告信息，可能存在潜在问题
    Error = 4,  ///< 错误信息，功能无法正常执行
    Fatal = 5,  ///< 致命错误，程序可能崩溃
};

}

#endif
