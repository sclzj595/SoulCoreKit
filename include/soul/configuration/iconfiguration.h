#ifndef SOUL_CONFIGURATION_ICONFIGURATION_H
#define SOUL_CONFIGURATION_ICONFIGURATION_H

#include <QString>
#include <variant>
#include "soul/core/result.h"
#include "soul/core/interface.h"

namespace sc {

/**
 * @class IConfiguration
 * @brief 配置接口定义
 *
 * IConfiguration 定义了统一的配置读取和写入接口，支持多种数据类型：
 * - QString 字符串
 * - int 整数
 * - double 浮点数
 * - bool 布尔值
 *
 * 具体实现包括：
 * - JsonConfiguration：JSON 文件配置
 *
 * 使用方式：
 * @code
 * std::shared_ptr<IConfiguration> config = std::make_shared<JsonConfiguration>();
 * auto result = config->load("config.json");
 * if (result.isOk()) {
 *     QString host = config->getString("server.host");
 *     int port = config->getInt("server.port", 8080);
 * }
 * @endcode
 *
 * @see JsonConfiguration
 */
class IConfiguration : public IInterface {
public:
    /**
     * @brief 虚析构函数
     */
    ~IConfiguration() override = default;

    /**
     * @brief 从文件加载配置
     * @param filePath 配置文件路径
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> load(const QString& filePath) = 0;

    /**
     * @brief 保存配置到文件
     * @param filePath 配置文件路径
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> save(const QString& filePath) = 0;

    /**
     * @brief 获取字符串配置
     * @param key 配置键（支持点号分隔，如 "server.host"）
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    virtual QString getString(const QString& key, const QString& defaultValue = "") const = 0;

    /**
     * @brief 获取整数配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    virtual int getInt(const QString& key, int defaultValue = 0) const = 0;

    /**
     * @brief 获取浮点数配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    virtual double getDouble(const QString& key, double defaultValue = 0.0) const = 0;

    /**
     * @brief 获取布尔配置
     * @param key 配置键
     * @param defaultValue 默认值
     * @return 配置值或默认值
     */
    virtual bool getBool(const QString& key, bool defaultValue = false) const = 0;

    /**
     * @brief 设置字符串配置
     * @param key 配置键
     * @param value 配置值
     */
    virtual void setString(const QString& key, const QString& value) = 0;

    /**
     * @brief 设置整数配置
     * @param key 配置键
     * @param value 配置值
     */
    virtual void setInt(const QString& key, int value) = 0;

    /**
     * @brief 设置浮点数配置
     * @param key 配置键
     * @param value 配置值
     */
    virtual void setDouble(const QString& key, double value) = 0;

    /**
     * @brief 设置布尔配置
     * @param key 配置键
     * @param value 配置值
     */
    virtual void setBool(const QString& key, bool value) = 0;

    /**
     * @brief 检查配置键是否存在
     * @param key 配置键
     * @return 如果存在返回 true
     */
    virtual bool contains(const QString& key) const = 0;

    /**
     * @brief 删除配置键
     * @param key 配置键
     */
    virtual void remove(const QString& key) = 0;

    std::string interfaceName() const override { return "IConfiguration"; }
};

}

#endif
