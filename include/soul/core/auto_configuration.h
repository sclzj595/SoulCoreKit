#ifndef SOUL_CORE_AUTO_CONFIGURATION_H
#define SOUL_CORE_AUTO_CONFIGURATION_H

// ============================================================================
// auto_configuration.h — 自动装配条件注解 [v2.0.0]
// ============================================================================
//
// 对标 SpringBoot 的 @ConditionalOnClass / @ConditionalOnProperty 等条件注解。
// 与 Module::isEnabled() 配合，让模块根据配置/类是否存在自动决定是否装配。
//
// CS 架构扩展 [v2.0.0]:
//   - conditionalOnDatabase(type): 当指定数据库类型驱动可用时装配
//   - conditionalOnDriverAvailable(type): 当驱动可用时装配
//   - conditionalOnCsMode(): 当 CS 模式运行时的条件装配
//   - datasourceConfigured(): 检查数据源是否已配置
//
// 用法:
//   class MyModule : public Module {
//   public:
//       MyModule() : Module("MyModule") {}
//       bool isEnabled() const override {
//           return conditionalOnProperty("my.module.enabled", true);
//       }
//   };
//
//   // CS 架构: 数据库模块根据配置自动装配
//   class DatabaseModule : public Module {
//   public:
//       DatabaseModule() : Module("DatabaseModule") {}
//       bool isEnabled() const override {
//           return datasourceConfigured();
//       }
//   };

// v3.0.0: 迁移到 ConfigSnapshot
#include <string>
#include <QSqlDatabase>
#include "soul/configuration/iconfig_provider.h"

namespace sc {

// ============================================================================
// 条件装配工具函数 [v3.0.0: 使用 ConfigSnapshot]
// ============================================================================

/// @brief 当配置属性存在且为指定值时启用
/// @param key 配置键
/// @param expectedValue 期望值(默认 true)
/// @param snapshot 配置快照 (可选，nullptr 时返回 false)
inline bool conditionalOnProperty(const std::string& key, bool expectedValue = true,
                                   const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return false;
    return snapshot->getBoolOr(QString::fromStdString(key), false) == expectedValue;
}

/// @brief 当配置属性存在时启用(不关心值) [v3.0.0]
inline bool conditionalOnPropertyExists(const std::string& key,
                                         const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return false;
    return snapshot->contains(QString::fromStdString(key));
}

/// @brief 当配置属性缺失时启用 [v3.0.0]
inline bool conditionalOnMissingProperty(const std::string& key,
                                          const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return true;
    return !snapshot->contains(QString::fromStdString(key));
}

/// @brief 当指定 Profile 激活时启用 [v3.0.0]
inline bool conditionalOnProfile(const std::string& profile,
                                  const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return false;
    return snapshot->getStringOr("app.profile", "") == QString::fromStdString(profile);
}

/// @brief 当非指定 Profile 时启用 [v3.0.0]
inline bool conditionalOnNotProfile(const std::string& profile,
                                     const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return true;
    return snapshot->getStringOr("app.profile", "") != QString::fromStdString(profile);
}

// ============================================================================
// CS 架构数据库条件装配 [v2.0.0 新增]
// ============================================================================

/// @brief 当指定数据库类型配置时启用
/// @param type 数据库类型("sqlite", "mysql", "postgresql", "mssql", "oracle")
///
/// @par 使用示例
/// @code
/// class MySqlModule : public Module {
///     bool isEnabled() const override {
///         return conditionalOnDatabase("mysql");
///     }
/// };
/// @endcode
inline bool conditionalOnDatabase(const std::string& type,
                                   const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return false;
    return snapshot->getStringOr("datasource.type", "") == QString::fromStdString(type);
}

/// @brief 当数据源已配置时启用(不关心具体类型)
///
/// @par 使用示例
/// @code
/// class OrmModule : public Module {
///     bool isEnabled() const override {
///         return datasourceConfigured();
///     }
/// };
/// @endcode
inline bool datasourceConfigured(const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return false;
    return snapshot->contains("datasource.type");
}

/// @brief 当指定的数据库驱动可用时启用
/// @param type 驱动类型("QSQLITE", "QMYSQL", "QPSQL", "QODBC", "QOCI")
///
/// @note 检查 Qt SQL 驱动是否可用
/// @par 使用示例
/// @code
/// class PostgresAutoConfig : public Module {
///     bool isEnabled() const override {
///         return conditionalOnDriverAvailable("QPSQL");
///     }
/// };
/// @endcode
inline bool conditionalOnDriverAvailable(const std::string& type) {
    return QSqlDatabase::isDriverAvailable(QString::fromStdString(type));
}

/// @brief 当运行在 CS 模式时启用(相对于 Web 模式)
///
/// 对于 CS 架构桌面应用，此条件始终为 true。
/// 保留此接口用于未来可能在 CS 和 Web 模式间切换的场景。
inline bool conditionalOnCsMode() {
    return true;
}

// ============================================================================
// Profile 工具
// ============================================================================

/// @brief 获取当前激活的 Profile
/// @param snapshot 配置快照 (可选, nullptr 时返回空字符串)
inline std::string activeProfile(const ConfigSnapshot* snapshot = nullptr) {
    if (!snapshot) return "";
    return snapshot->getStringOr("app.profile", "").toStdString();
}

/// @brief 检查是否为开发环境
inline bool isDevProfile(const ConfigSnapshot* snapshot = nullptr) {
    return activeProfile(snapshot) == "dev";
}

/// @brief 检查是否为生产环境
inline bool isProdProfile(const ConfigSnapshot* snapshot = nullptr) {
    return activeProfile(snapshot) == "prod";
}

/// @brief 检查是否为测试环境
inline bool isTestProfile(const ConfigSnapshot* snapshot = nullptr) {
    return activeProfile(snapshot) == "test";
}

} // namespace sc

#endif // SOUL_CORE_AUTO_CONFIGURATION_H