#ifndef SOUL_CORE_CONFIGURATION_H
#define SOUL_CORE_CONFIGURATION_H

// ============================================================================
// configuration.h — YAML 配置加载器 [v2.0.0]
// ============================================================================
//
// 对标 SpringBoot 的 application.yml 配置机制。
// 支持分层键值读取、Profile 环境隔离、命令行参数覆盖。
//
// 用法:
//   auto& cfg = Configuration::instance();
//   cfg.loadFromFile("application.yml");
//   int port = cfg.get<int>("server.port", 8080);
//   std::string dbPath = cfg.get<std::string>("datasource.path", ":memory:");
//
// 配置优先级(从高到低):
//   1. 命令行参数 (--server.port=9090)
//   2. application-{profile}.yml
//   3. application.yml
//   4. 默认值

#include <QString>
#include <QVariant>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace sc {

class Configuration {
public:
    static Configuration& instance();

    Configuration(const Configuration&) = delete;
    Configuration& operator=(const Configuration&) = delete;

    // ========================================================================
    // 文件加载
    // ========================================================================

    /// @brief 从 YAML 文件加载配置
    /// @param filePath 配置文件路径
    /// @return 是否加载成功
    bool loadFromFile(const std::string& filePath);

    /// @brief 从 YAML 字符串加载配置(用于测试)
    void loadFromString(const std::string& yamlContent);

    /// @brief 设置当前激活的 Profile
    /// @param profile Profile 名称 (如 "dev", "prod")
    void setActiveProfile(const std::string& profile);

    /// @brief 获取当前激活的 Profile
    std::string activeProfile() const;

    // ========================================================================
    // 值读取
    // ========================================================================

    /// @brief 获取配置值(带默认值)
    /// @param key 点分键路径 (如 "server.port", "datasource.host")
    /// @param defaultValue 默认值
    template<typename T>
    T get(const std::string& key, const T& defaultValue = T{}) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_values.find(key);
        if (it != m_values.end()) {
            QVariant v = it->second;
            if (v.canConvert<T>()) {
                return v.value<T>();
            }
        }
        return defaultValue;
    }

    /// @brief 检查配置键是否存在
    bool contains(const std::string& key) const;

    /// @brief 获取所有配置键
    std::vector<std::string> keys() const;

    /// @brief 获取所有配置(用于 /actuator/env)
    std::map<std::string, std::string> all() const;

    // ========================================================================
    // 命令行参数覆盖
    // ========================================================================

    /// @brief 解析命令行参数并覆盖配置
    /// @param argc 参数数量
    /// @param argv 参数数组
    void parseCommandLine(int argc, char* argv[]);

    // ========================================================================
    // 便捷方法
    // ========================================================================

    /// @brief 获取 server.port
    int serverPort() const { return get<int>("server.port", 8080); }

    /// @brief 获取 server.host
    std::string serverHost() const { return get<std::string>("server.host", "0.0.0.0"); }

    /// @brief 获取 datasource 配置
    std::string databasePath() const { return get<std::string>("datasource.path", ":memory:"); }

    /// @brief 获取 logging.level
    std::string logLevel() const { return get<std::string>("logging.level", "info"); }

    /// @brief 清空所有配置(主要用于测试)
    void clear();

private:
    Configuration() = default;
    ~Configuration() = default;

    /// @brief 解析 YAML 内容到 m_values
    void parseYaml(const std::string& content, const std::string& prefix = "");

    /// @brief 存储单个值（自动类型推断）[v2.0.0]
    void storeValue(const std::string& key, const std::string& value);

    mutable std::mutex m_mutex;
    std::map<std::string, QVariant> m_values;
    std::string m_activeProfile;
};

// ============================================================================
// 模板特化: std::string 的 QVariant 转换
// ============================================================================
// QVariant 默认不支持 std::string,需要显式转换
// Configuration::get<std::string> 使用 QString 中间转换

} // namespace sc

#endif // SOUL_CORE_CONFIGURATION_H