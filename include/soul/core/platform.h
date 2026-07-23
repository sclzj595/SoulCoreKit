#ifndef SOUL_CORE_PLATFORM_H
#define SOUL_CORE_PLATFORM_H

#include <string>

namespace sc {

/**
 * @enum OS
 * @brief 操作系统类型枚举
 */
enum class OS {
    Unknown,   ///< 未知操作系统
    Windows,   ///< Windows 系统
    macOS,     ///< macOS 系统
    Linux,     ///< Linux 系统
};

/**
 * @enum RuntimeMode
 * @brief 运行时模式枚举
 */
enum class RuntimeMode {
    Development, ///< 开发模式
    Production,  ///< 生产模式
    Testing,     ///< 测试模式
};

/**
 * @class Platform
 * @brief 平台检测与运行时环境工具类
 *
 * Platform 类提供跨平台的系统信息检测功能，包括：
 * - 操作系统类型和版本检测
 * - CPU架构检测
 * - 运行时模式（开发/生产/测试）管理
 * - 构建类型（Debug/Release）检测
 * - 系统资源信息获取
 */
class Platform {
public:
    /**
     * @brief 获取操作系统类型
     * @return 操作系统类型枚举值
     */
    static OS os();

    /**
     * @brief 获取操作系统名称
     * @return 操作系统名称字符串
     */
    static std::string osName();

    /**
     * @brief 获取操作系统版本
     * @return 操作系统版本字符串
     */
    static std::string osVersion();

    /**
     * @brief 获取CPU架构
     * @return CPU架构字符串（如 x86_64, arm64）
     */
    static std::string architecture();

    /**
     * @brief 获取当前运行时模式
     * @return 运行时模式枚举值
     */
    static RuntimeMode runtimeMode();

    /**
     * @brief 设置运行时模式
     * @param mode 新的运行时模式
     */
    static void setRuntimeMode(RuntimeMode mode);

    /**
     * @brief 检查是否为Debug构建
     * @return 如果是Debug构建返回 true
     */
    static bool isDebugBuild();

    /**
     * @brief 检查是否为Release构建
     * @return 如果是Release构建返回 true
     */
    static bool isReleaseBuild();

    /**
     * @brief 获取CPU信息
     * @return CPU信息字符串
     */
    static std::string cpuInfo();

    /**
     * @brief 获取内存大小
     * @return 内存大小（字节）
     */
    static size_t memorySize();
};

}

#endif
