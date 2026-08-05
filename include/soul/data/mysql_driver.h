#ifndef SOUL_DATA_MYSQL_DRIVER_H
#define SOUL_DATA_MYSQL_DRIVER_H

// ============================================================================
// mysql_driver.h — MySQL 数据库驱动 [v2.0.0 新增]
// ============================================================================
//
// 基于 DatabaseDriverBase 模板基类实现 MySQL 专用驱动。
// 使用 Qt 的 QMYSQL 驱动，提供 MySQL 特有的功能支持。
//
// 核心设计:
//   - 继承 DatabaseDriverBase<MysqlDriver>，复用公共 SQL 执行逻辑
//   - 实现 open() 和 getType() 钩子方法
//   - 支持 MySQL 特有功能：tableExists() 通过 INFORMATION_SCHEMA 查询
//   - executeMigrate() 直接委托给 executeUpdate()
//
// 用法:
//   MysqlDriver driver;
//   ConnectionConfig config;
//   config.host = "localhost";
//   config.port = 3306;
//   config.database = "mydb";
//   config.username = "root";
//   config.password = "secret";
//   driver.open(config);

#include <QString>
#include <QVariant>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <string>
#include <vector>
#include "soul/data/database_driver.h"
#include "soul/data/database_driver_base.h"
#include "soul/core/result.h"
#include "soul/core/error.h"

namespace sc {
namespace data {

// ============================================================================
// MysqlDriver — MySQL 数据库驱动
// ============================================================================
///
/// @brief MySQL 数据库驱动实现
///
/// 使用 Qt QMYSQL 驱动连接 MySQL 数据库。
/// 通过 DatabaseDriverBase 模板基类复用公共 SQL 执行逻辑。
///
/// @par 连接配置
///   - host:     数据库主机地址
///   - port:     端口（默认 3306）
///   - database: 数据库名
///   - username: 用户名
///   - password: 密码
///
/// @thread_safety 非线程安全 — 单线程使用
class MysqlDriver : public DatabaseDriverBase<MysqlDriver> {
public:
    MysqlDriver() = default;
    ~MysqlDriver() override = default;

    // ========================================================================
    // 连接管理
    // ========================================================================

    /// @brief 打开 MySQL 数据库连接
    /// @param config 连接配置
    /// @return 成功返回 Ok，失败返回错误信息
    Result<void> open(const ConnectionConfig& config) override {
        if (isConnected()) {
            (void)close();
        }

        generateConnectionId();
        QString dbName = QString::fromStdString(m_connectionId);

        m_db = QSqlDatabase::addDatabase("QMYSQL", dbName);
        m_db.setHostName(config.host);
        m_db.setPort(config.port > 0 ? config.port : 3306);
        m_db.setDatabaseName(config.database);
        m_db.setUserName(config.username);
        m_db.setPassword(config.password);

        if (config.connectionTimeoutMs > 0) {
            m_db.setConnectOptions(
                QStringLiteral("connect_timeout=%1").arg(config.connectionTimeoutMs / 1000));
        }

        if (!m_db.open()) {
            return Error(ErrorCode::DatabaseError, m_db.lastError().text());
        }

        return {};
    }

    /// @return 数据库类型
    DatabaseType getType() const override { return DatabaseType::MySQL; }

    // ========================================================================
    // MySQL 特有功能
    // ========================================================================

    /// @brief 检查表是否存在
    /// @param tableName 表名
    /// @return true=表存在
    ///
    /// @note 通过查询 INFORMATION_SCHEMA.TABLES 系统表实现
    Result<bool> tableExists(const QString& tableName) override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }

        QString sql = QStringLiteral(
            "SELECT EXISTS ("
            "  SELECT 1 FROM INFORMATION_SCHEMA.TABLES "
            "  WHERE TABLE_SCHEMA = DATABASE() "
            "    AND TABLE_NAME = ?"
            ")");
        std::vector<QVariant> params = {tableName};

        auto result = executeQuery(sql, params);
        if (!result.isOk()) {
            return Error(ErrorCode::QueryFailed, "Failed to check table existence");
        }

        auto& rows = result.unwrap().rows;
        if (!rows.empty()) {
            auto it = rows[0].find("exists");
            if (it != rows[0].end()) {
                return it->second.toBool();
            }
        }
        return false;
    }
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_MYSQL_DRIVER_H