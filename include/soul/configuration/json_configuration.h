#ifndef SOUL_CONFIGURATION_JSON_CONFIGURATION_H
#define SOUL_CONFIGURATION_JSON_CONFIGURATION_H

#include "iconfiguration.h"
#include <QJsonObject>

namespace sc {

/**
 * @class JsonConfiguration
 * @brief JSON 文件配置实现
 *
 * JsonConfiguration 使用 Qt 的 QJsonObject 作为底层存储，
 * 支持从 JSON 文件加载和保存配置。
 *
 * 使用方式：
 * @code
 * JsonConfiguration config;
 * config.load("config.json");
 * QString host = config.getString("server.host");
 * config.setInt("server.port", 8080);
 * config.save("config.json");
 * @endcode
 *
 * @see IConfiguration
 */
class JsonConfiguration : public IConfiguration {
public:
    /**
     * @brief 构造函数
     */
    JsonConfiguration();

    /**
     * @brief 析构函数
     */
    ~JsonConfiguration() override = default;

    /**
     * @brief 从 JSON 文件加载配置
     * @param filePath JSON 文件路径
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    Result<void> load(const QString& filePath) override;

    /**
     * @brief 保存配置到 JSON 文件
     * @param filePath JSON 文件路径
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    Result<void> save(const QString& filePath) override;

    /**
     * @brief 获取字符串配置
     * @param key 配置键（支持点号分隔）
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    QString getString(const QString& key, const QString& defaultValue = "") const override;

    /**
     * @brief 获取整数配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    int getInt(const QString& key, int defaultValue = 0) const override;

    /**
     * @brief 获取浮点数配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    double getDouble(const QString& key, double defaultValue = 0.0) const override;

    /**
     * @brief 获取布尔配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    bool getBool(const QString& key, bool defaultValue = false) const override;

    /**
     * @brief 设置字符串配置
     * @param key 配置键
     * @param value 配置值
     */
    void setString(const QString& key, const QString& value) override;

    /**
     * @brief 设置整数配置
     * @param key 配置键
     * @param value 配置值
     */
    void setInt(const QString& key, int value) override;

    /**
     * @brief 设置浮点数配置
     * @param key 配置键
     * @param value 配置值
     */
    void setDouble(const QString& key, double value) override;

    /**
     * @brief 设置布尔配置
     * @param key 配置键
     * @param value 配置值
     */
    void setBool(const QString& key, bool value) override;

    /**
     * @brief 检查配置键是否存在
     * @param key 配置键
     * @return 如果存在返回 true
     */
    bool contains(const QString& key) const override;

    /**
     * @brief 删除配置键
     * @param key 配置键
     */
    void remove(const QString& key) override;

    /**
     * @brief 获取底层 JSON 数据
     * @return QJsonObject 引用
     */
    const QJsonObject& data() const;

private:
    QJsonObject m_data;
};

}

#endif
