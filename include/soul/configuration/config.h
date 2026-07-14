#ifndef SOUL_CONFIGURATION_CONFIG_H
#define SOUL_CONFIGURATION_CONFIG_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QFileSystemWatcher>
#include <vector>
#include <memory>
#include <functional>
#include "soul/core/singleton.h"
#include "soul/core/result.h"

namespace sc {

class IConfiguration;

/**
 * @class Config
 * @brief 配置管理类，支持多源配置和热加载
 *
 * Config 是配置系统的入口类，采用单例模式。支持：
 * - 从目录加载多个配置文件
 * - 从环境变量加载配置
 * - 配置热加载（文件变更自动重新加载）
 * - 配置变更回调通知
 *
 * 使用方式：
 * @code
 * Config::instance().loadFromDirectory("config/");
 * QString host = Config::instance().getString("server.host", "localhost");
 * int port = Config::instance().getInt("server.port", 8080);
 * @endcode
 *
 * @see IConfiguration, ConfigSchema
 */
class Config : public QObject, public Singleton<Config> {
    Q_OBJECT
    friend class Singleton<Config>;
public:
    /**
     * @brief 初始化配置管理器
     */
    void init();

    /**
     * @brief 关闭配置管理器
     */
    void shutdown();

    /**
     * @brief 配置变更回调类型
     * @param key 变更的配置键
     */
    using ConfigChangeCallback = std::function<void(const QString& key)>;

    /**
     * @brief 从目录加载配置
     * @param configDir 配置目录路径
     * @return Result<void>，成功返回 Ok
     */
    Result<void> loadFromDirectory(const QString& configDir);

    /**
     * @brief 加载单个配置文件
     * @param filePath 配置文件路径
     * @return Result<void>，成功返回 Ok
     */
    Result<void> loadFile(const QString& filePath);

    /**
     * @brief 从环境变量加载配置
     * @param env 环境标识（如 "dev", "prod"）
     * @return Result<void>，成功返回 Ok
     */
    Result<void> loadEnvironment(const QString& env);

    /**
     * @brief 获取字符串配置
     * @param key 配置键（支持点号分隔）
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    QString getString(const QString& key, const QString& defaultValue = "") const;

    /**
     * @brief 获取整数配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    int getInt(const QString& key, int defaultValue = 0) const;

    /**
     * @brief 获取浮点数配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    double getDouble(const QString& key, double defaultValue = 0.0) const;

    /**
     * @brief 获取布尔配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    bool getBool(const QString& key, bool defaultValue = false) const;

    /**
     * @brief 设置字符串配置
     * @param key 配置键
     * @param value 配置值
     */
    void setString(const QString& key, const QString& value);

    /**
     * @brief 设置整数配置
     * @param key 配置键
     * @param value 配置值
     */
    void setInt(const QString& key, int value);

    /**
     * @brief 设置浮点数配置
     * @param key 配置键
     * @param value 配置值
     */
    void setDouble(const QString& key, double value);

    /**
     * @brief 设置布尔配置
     * @param key 配置键
     * @param value 配置值
     */
    void setBool(const QString& key, bool value);

    /**
     * @brief 检查配置键是否存在
     * @param key 配置键
     * @return 如果存在返回 true
     */
    bool contains(const QString& key) const;

    /**
     * @brief 删除配置键
     * @param key 配置键
     */
    void remove(const QString& key);

    /**
     * @brief 保存配置到文件
     * @param filePath 目标文件路径
     * @return Result<void>，成功返回 Ok
     */
    Result<void> saveToFile(const QString& filePath) const;

    /**
     * @brief 保存所有配置
     * @return Result<void>，成功返回 Ok
     */
    Result<void> saveAll() const;

    /**
     * @brief 启用/禁用配置热加载
     * @param enabled 是否启用热加载
     */
    void setHotReloadEnabled(bool enabled);

    /**
     * @brief 检查是否启用热加载
     * @return 如果启用返回 true
     */
    bool isHotReloadEnabled() const;

    /**
     * @brief 添加配置变更回调
     * @param callback 回调函数
     */
    void addChangeCallback(ConfigChangeCallback callback);

    /**
     * @brief 移除配置变更回调
     * @param callback 回调函数
     */
    void removeChangeCallback(ConfigChangeCallback callback);

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Config();

    /**
     * @brief 析构函数
     */
    ~Config() override;

    std::vector<std::shared_ptr<IConfiguration>> m_configSources;
    QString m_configDir;
    QString m_currentEnv;
    bool m_hotReloadEnabled = false;
    QFileSystemWatcher m_fileWatcher;
    std::vector<ConfigChangeCallback> m_changeCallbacks;

    bool loadJsonFile(const QString& filePath);
    void onFileChanged(const QString& path);
    QVariant getValue(const QString& key) const;
    void setValue(const QString& key, const QVariant& value);
};

}

#endif
