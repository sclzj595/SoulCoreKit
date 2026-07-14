#ifndef SOUL_CORE_ENVIRONMENT_H
#define SOUL_CORE_ENVIRONMENT_H

#include <string>
#include <unordered_map>

namespace sc {

/**
 * @class Environment
 * @brief 环境变量与系统路径工具类
 *
 * Environment 类提供访问和管理环境变量、系统路径的工具方法，包括：
 * - 环境变量的读取、设置和检查
 * - 可执行文件路径、工作目录、用户目录等系统路径的获取
 * - 命令行参数解析
 */
class Environment {
public:
    /**
     * @brief 获取环境变量值
     * @param key 环境变量名称
     * @param defaultValue 默认值（当环境变量不存在时返回）
     * @return 环境变量值或默认值
     */
    static std::string get(const std::string& key, const std::string& defaultValue = "");

    /**
     * @brief 设置环境变量值
     * @param key 环境变量名称
     * @param value 环境变量值
     */
    static void set(const std::string& key, const std::string& value);

    /**
     * @brief 检查环境变量是否存在
     * @param key 环境变量名称
     * @return 如果存在返回 true，否则返回 false
     */
    static bool contains(const std::string& key);

    /**
     * @brief 获取当前可执行文件路径
     * @return 可执行文件的完整路径
     */
    static std::string getExecutablePath();

    /**
     * @brief 获取当前工作目录
     * @return 当前工作目录路径
     */
    static std::string getWorkingDirectory();

    /**
     * @brief 设置当前工作目录
     * @param path 新的工作目录路径
     */
    static void setWorkingDirectory(const std::string& path);

    /**
     * @brief 获取用户主目录
     * @return 用户主目录路径
     */
    static std::string getHomeDirectory();

    /**
     * @brief 获取应用数据目录
     * @return 应用数据目录路径（如 ~/.local/share 或 %APPDATA%）
     */
    static std::string getAppDataDirectory();

    /**
     * @brief 获取临时目录
     * @return 系统临时目录路径
     */
    static std::string getTempDirectory();

    /**
     * @brief 获取环境变量字符串表示
     * @return 所有环境变量的字符串表示
     */
    static std::string getEnv();

    /**
     * @brief 解析命令行参数
     * @param argc 参数数量
     * @param argv 参数数组
     * @return 解析后的键值对映射
     */
    static std::unordered_map<std::string, std::string> parseCommandLine(int argc, char* argv[]);
};

}

#endif
