#ifndef SOUL_CONFIGURATION_CONFIG_SCHEMA_H
#define SOUL_CONFIGURATION_CONFIG_SCHEMA_H

#include <QString>
#include <QVariant>
#include <functional>
#include <vector>
#include <memory>

namespace sc {

/**
 * @enum ConfigType
 * @brief 配置字段类型枚举
 */
enum class ConfigType {
    String,      ///< 字符串类型
    Int,         ///< 整数类型
    Double,      ///< 浮点数类型
    Bool,        ///< 布尔类型
    StringList,  ///< 字符串列表类型
    IntList,     ///< 整数列表类型
    Object       ///< 对象类型
};

/**
 * @struct ConfigField
 * @brief 配置字段定义结构体
 *
 * ConfigField 定义了单个配置字段的元信息，包括名称、类型、默认值、
 * 必填标志、验证器和描述。
 */
struct ConfigField {
    QString name;                           ///< 字段名称
    ConfigType type;                        ///< 字段类型
    QVariant defaultValue;                  ///< 默认值
    bool required = false;                  ///< 是否必填
    std::function<bool(const QVariant&)> validator; ///< 自定义验证器
    QString description;                    ///< 字段描述
};

/**
 * @class ConfigSchema
 * @brief 配置模式验证类
 *
 * ConfigSchema 用于定义配置的结构和验证规则，支持：
 * - 定义配置字段（名称、类型、默认值、必填）
 * - 自定义字段验证器
 * - 验证配置数据是否符合模式
 * - 应用默认值到配置数据
 *
 * 使用方式：
 * @code
 * ConfigSchema schema;
 * schema.addField({
 *     "server.host",
 *     ConfigType::String,
 *     "localhost",
 *     true,
 *     [](const QVariant& v) { return !v.toString().isEmpty(); },
 *     "服务器地址"
 * });
 *
 * QString error;
 * bool valid = schema.validate(configMap, &error);
 * @endcode
 */
class ConfigSchema {
public:
    /**
     * @brief 构造函数
     */
    ConfigSchema();

    /**
     * @brief 添加配置字段定义
     * @param field 字段定义
     */
    void addField(const ConfigField& field);

    /**
     * @brief 验证配置数据是否符合模式
     * @param config 配置数据
     * @param errorMsg 错误消息输出（可选）
     * @return 如果验证通过返回 true
     */
    bool validate(const QVariantMap& config, QString* errorMsg = nullptr) const;

    /**
     * @brief 为配置数据应用默认值
     * @param config 配置数据
     * @return 应用默认值后的配置数据
     */
    QVariantMap applyDefaults(const QVariantMap& config) const;

    /**
     * @brief 获取所有字段定义
     * @return 字段定义列表
     */
    const std::vector<ConfigField>& fields() const;

private:
    std::vector<ConfigField> m_fields;
};

}

#endif
