#ifndef SOUL_CONFIGURATION_CONFIG_H
#define SOUL_CONFIGURATION_CONFIG_H

#include <QObject>
#include <QString>
#include <QFileSystemWatcher>
#include <QVariantMap>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <QRecursiveMutex>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <set>
#include <atomic>
#include "soul/core/singleton.h"
#include "soul/core/result.h"
#include "soul/configuration/config_schema.h"
#include "soul/configuration/config_bind.h"

namespace sc {

class IConfiguration;

/**
 * @class Config
 * @brief 配置管理类，支持多源配置和热加载
 *
 * Config 是配置系统的入口类，采用单例模式。支持：
 * - 从目录加载多个配置文件
 * - 从环境变量加载配置（支持环境变量覆盖）
 * - 配置热加载（文件变更自动重新加载）
 * - 配置变更回调通知
 * - 配置验证（基于 ConfigSchema）
 * - 配置分组获取
 *
 * 使用方式：
 * @code
 * Config::instance().loadFromDirectory("config/");
 * QString host = Config::instance().getString("server.host", "localhost");
 * int port = Config::instance().getInt("server.port", 8080);
 *
 * // 环境变量覆盖：SOUL_SERVER_PORT=9090
 * int envPort = Config::instance().getInt("server.port", 8080); // 9090
 *
 * // 获取配置分组
 * QVariantMap serverConfig = Config::instance().getGroup("server");
 * @endcode
 *
 * @see IConfiguration, ConfigSchema
 */
class Config : public QObject, public Singleton<Config> {
    Q_OBJECT
    friend class Singleton<Config>;
signals:
    /**
     * @brief 配置变更信号
     * @param key 变更的配置键
     * @param value 变更后的配置值
     */
    void configChanged(const QString& key, const QVariant& value);

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
     * @brief 获取字符串配置（优先从环境变量读取）
     * @param key 配置键（支持点号分隔）
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    QString getString(const QString& key, const QString& defaultValue = "") const;

    /**
     * @brief 获取整数配置（优先从环境变量读取）
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    int getInt(const QString& key, int defaultValue = 0) const;

    /**
     * @brief 获取浮点数配置（优先从环境变量读取）
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    double getDouble(const QString& key, double defaultValue = 0.0) const;

    /**
     * @brief 获取布尔配置（优先从环境变量读取）
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    bool getBool(const QString& key, bool defaultValue = false) const;

    /**
     * @brief 获取配置分组
     * @param group 分组名称（如 "server"）
     * @return 分组下的所有配置
     */
    QVariantMap getGroup(const QString& group) const;

    /**
     * @brief 获取所有配置
     * @return 所有配置的映射
     */
    QVariantMap getAll() const;

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
     * @return 回调令牌,用于后续 removeChangeCallback 移除
     * @note token 从 1 开始,0 表示无效令牌
     */
    std::size_t addChangeCallback(ConfigChangeCallback callback);

    /**
     * @brief 移除配置变更回调
     * @param token addChangeCallback 返回的令牌
     * @return 成功移除返回 true,token 无效返回 false
     */
    bool removeChangeCallback(std::size_t token);

    /**
     * @brief 验证配置是否符合模式
     * @param schema 配置模式
     * @param errorMsg 错误消息输出（可选）
     * @return 如果验证通过返回 true
     */
    bool validate(const ConfigSchema& schema, QString* errorMsg = nullptr) const;

    /**
     * @brief 设置环境变量前缀（默认 "SOUL_"）
     * @param prefix 环境变量前缀
     */
    void setEnvPrefix(const QString& prefix);

    /**
     * @brief 获取环境变量前缀
     * @return 环境变量前缀
     */
    QString envPrefix() const;

    // --- Profile 环境隔离(对标 SpringBoot application-{profile}.yml) ---

    /**
     * @brief 设置当前 Profile(如 "dev"/"test"/"prod")
     *
     * 设置后会优先加载 application-{profile}.json 配置文件,
     * 并在 getString/getInt 等查询时优先从 profile 命名空间返回值。
     *
     * @param profile Profile 名称
     */
    void setProfile(const QString& profile);

    /**
     * @brief 获取当前 Profile
     * @return Profile 名称(未设置时返回空)
     */
    QString profile() const;

    /**
     * @brief 加载指定 Profile 的配置文件
     *
     * 从已加载的配置目录中查找 application-{profile}.json 并合并到当前配置。
     * 覆盖优先级: 环境变量 > profile 文件 > 主配置文件 > 默认值
     *
     * @param profile Profile 名称
     * @return Result<void>,成功返回 Ok
     */
    Result<void> loadProfile(const QString& profile);

    // --- 配置元数据绑定 [v1.9.1 新增] ---

    /**
     * @brief 类型安全配置绑定(对标 SpringBoot @ConfigurationProperties)
     *
     * 将 Config 中的 key-value 自动映射到 struct T 的字段上。
     * 需要先通过 SC_CONFIG_BIND 宏声明 T 的字段绑定元数据。
     *
     * @tparam T 目标 struct 类型(需通过 SC_CONFIG_BIND 声明绑定)
     * @param prefix 配置键前缀(如 "server"),最终查询 key = prefix + "." + fieldName
     * @return 绑定后的 struct 实例
     *
     * @par 使用示例
     * @code
     * struct ServerConfig {
     *     QString host;
     *     int port = 0;
     * };
     * SC_CONFIG_BIND(ServerConfig,
     *     SC_CFG_FIELD("host", &ServerConfig::host, "localhost")
     *     SC_CFG_FIELD("port", &ServerConfig::port, 8080)
     * )
     *
     * auto cfg = Config::instance().bind<ServerConfig>("server");
     * // cfg.host == "localhost", cfg.port == 8080
     * @endcode
     */
    template<typename T>
    T bind(const QString& prefix = QString()) const {
        T obj{};
        const auto& fields = ConfigBindTraits<T>::fields();
        if (fields.empty()) {
            return obj;
        }

        // 使用 traits 中的 prefix 或参数传入的 prefix
        QString effectivePrefix = prefix;
        if (effectivePrefix.isEmpty()) {
            const char* traitPrefix = ConfigBindTraits<T>::prefix();
            if (traitPrefix && traitPrefix[0] != '\0') {
                effectivePrefix = QString::fromUtf8(traitPrefix);
            }
        }

        // 加锁一次,遍历所有字段(利用 QRecursiveMutex 可重入特性)
        QMutexLocker lock(&m_dataMutex);
        for (const auto& field : fields) {
            QString fullKey = effectivePrefix.isEmpty()
                ? QString::fromStdString(field.key)
                : effectivePrefix + "." + QString::fromStdString(field.key);

            QVariant value = getValueLocked(fullKey);
            if (value.isValid()) {
                field.setter(obj, value);
            } else {
                // 使用默认值
                field.setter(obj, field.defaultValue);
            }
        }
        return obj;
    }

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Config();

    /**
     * @brief 析构函数
     */
    ~Config() override;

    /**
     * @brief 从环境变量获取配置键
     * @param key 配置键
     * @return 环境变量名
     */
    QString getEnvKey(const QString& key) const;

    /**
     * @brief 从环境变量获取值
     * @param key 配置键
     * @return 环境变量值（无效时返回空）
     */
    QString getEnvValue(const QString& key) const;

    std::vector<std::shared_ptr<IConfiguration>> m_configSources;
    QString m_configDir;
    QString m_currentEnv;
    QString m_profile;  // Profile 环境隔离(对标 SpringBoot application-{profile}.yml)
    bool m_hotReloadEnabled = false;
    QFileSystemWatcher m_fileWatcher;
    std::map<std::size_t, ConfigChangeCallback> m_changeCallbacks;  // token -> callback
    QString m_envPrefix = "SOUL_";
    QTimer m_debounceTimer;
    std::set<QString> m_pendingChanges;

    // 数据成员线程安全保护(对标 ADR-005 线程安全策略)
    // 保护范围: m_configSources / m_configDir / m_currentEnv / m_profile /
    //           m_hotReloadEnabled / m_changeCallbacks / m_envPrefix / m_pendingChanges
    // 注意: m_fileWatcher / m_debounceTimer 是 QObject,必须在所属线程访问,
    //       不由本锁保护(由 Qt 信号槽的队列连接保证线程安全)。
    mutable QRecursiveMutex m_dataMutex;
    std::atomic<std::size_t> m_nextCallbackToken{1};  // 回调令牌生成器(从 1 开始)

    bool loadJsonFileLocked(const QString& filePath);
    void onFileChanged(const QString& path);
    void processPendingChanges();
    QVariant getValueLocked(const QString& key) const;
    void setValueLocked(const QString& key, const QVariant& value);
};

}

#endif
