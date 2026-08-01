#ifndef SOUL_CONFIGURATION_CONFIG_BIND_H
#define SOUL_CONFIGURATION_CONFIG_BIND_H

// ============================================================================
// config_bind.h — 配置元数据绑定(对标 SpringBoot @ConfigurationProperties)
// ============================================================================
//
// 设计目标: 提供类型安全的配置绑定,将 Config key-value 自动映射到 C++ struct 字段。
// 消除手写 getString("server.host") 等字符串 key 的拼写错误风险。
//
// 设计原则:
//   - 编译期类型安全: 通过成员指针绑定字段类型
//   - 零样板: 一行宏声明所有字段映射
//   - 可复用: 利用 ORM 模块的 ReflectionTable 实现通用绑定
//   - 启动校验: 支持必填字段校验
//
// 用法:
//   struct ServerConfig {
//       QString host;
//       int port = 0;
//       bool tls = false;
//   };
//
//   // 在类外声明反射表(复用 ORM 的 ReflectionTable)
//   SC_CONFIG_BIND(ServerConfig,
//       SC_CFG_FIELD("host", &ServerConfig::host, "localhost")
//       SC_CFG_FIELD("port", &ServerConfig::port, 8080)
//       SC_CFG_FIELD("tls",  &ServerConfig::tls,  false)
//   )
//
//   // 绑定配置
//   auto cfg = Config::instance().bind<ServerConfig>("server");
//   // cfg.host == "localhost" (若未配置则使用默认值)
//   // cfg.port == 8080

#include <QString>
#include <QVariant>
#include <functional>
#include <mutex>
#include <string>
#include <vector>
#include <type_traits>

#include "soul/core/result.h"

namespace sc {

// ============================================================================
// ConfigFieldBinding — 单字段绑定描述
// ============================================================================
template<typename T>
struct ConfigFieldBinding {
    std::string key;                                    ///< 配置键(相对于 prefix)
    std::function<void(T&, const QVariant&)> setter;    ///< 字段设置器
    QVariant defaultValue;                              ///< 默认值
    bool required = false;                             ///< 是否必填
};

// ============================================================================
// ConfigBindTraits — 配置绑定元数据(需通过 SC_CONFIG_BIND 宏特化)
// ============================================================================
template<typename T>
struct ConfigBindTraits {
    /// @return 配置键前缀(如 "server")
    static const char* prefix() { return ""; }

    /// @return 字段绑定列表
    static std::vector<ConfigFieldBinding<T>> fields() { return {}; }

    /// @return 绑定后验证器(可选,返回 true 表示通过)
    static bool validate(const T&) { return true; }
};

// ============================================================================
// 辅助函数: 创建字段绑定(FieldType 与 DefaultType 分离,避免类型推导冲突)
// ============================================================================
template<typename T, typename FieldType, typename DefaultType>
ConfigFieldBinding<T> makeConfigField(
    const char* key, FieldType T::*member, DefaultType&& defaultVal, bool required = false)
{
    return {
        key,
        [member](T& obj, const QVariant& v) {
            obj.*member = v.value<FieldType>();
        },
        QVariant::fromValue(static_cast<FieldType>(std::forward<DefaultType>(defaultVal))),
        required
    };
}

// ============================================================================
// 宏: SC_CONFIG_BIND — 声明配置绑定元数据
// ============================================================================
//
// 用法:
//   SC_CONFIG_BIND(ServerConfig,
//       SC_CFG_FIELD("host", &ServerConfig::host, "localhost")
//       SC_CFG_FIELD("port", &ServerConfig::port, 8080)
//   )
//
// 展开为 ConfigBindTraits<ServerConfig> 的显式特化。
// 注意: 必须在全局命名空间内使用(跟在 struct 定义之后)。
//
// 设计说明: 使用 push_back + std::call_once 模式而非 initializer_list,
// 避免宏展开中逗号导致 initializer_list 元素解析错误。
#define SC_CONFIG_BIND(StructName, Fields) \
    template<> struct sc::ConfigBindTraits<StructName> { \
        static const char* prefix() { return ""; } \
        static std::vector<sc::ConfigFieldBinding<StructName>> fields() { \
            static std::vector<sc::ConfigFieldBinding<StructName>> _fields; \
            static std::once_flag _flag; \
            std::call_once(_flag, [&]() { \
                Fields \
            }); \
            return _fields; \
        } \
        static bool validate(const StructName&) { return true; } \
    };

/// @brief 带前缀的配置绑定宏
#define SC_CONFIG_BIND_PREFIX(StructName, Prefix, Fields) \
    template<> struct sc::ConfigBindTraits<StructName> { \
        static const char* prefix() { return Prefix; } \
        static std::vector<sc::ConfigFieldBinding<StructName>> fields() { \
            static std::vector<sc::ConfigFieldBinding<StructName>> _fields; \
            static std::once_flag _flag; \
            std::call_once(_flag, [&]() { \
                Fields \
            }); \
            return _fields; \
        } \
        static bool validate(const StructName&) { return true; } \
    };

/// @brief 声明单个字段绑定(向 _fields 追加)
#define SC_CFG_FIELD(key, member, defaultVal) \
    _fields.push_back(::sc::makeConfigField(key, member, defaultVal));

/// @brief 声明必填字段绑定
#define SC_CFG_FIELD_REQUIRED(key, member, defaultVal) \
    _fields.push_back(::sc::makeConfigField(key, member, defaultVal, true));

} // namespace sc

#endif // SOUL_CONFIGURATION_CONFIG_BIND_H