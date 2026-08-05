#ifndef SOUL_DATA_POSTGRES_DRIVER_H
#define SOUL_DATA_POSTGRES_DRIVER_H

// ============================================================================
// postgres_driver.h — PostgreSQL 数据库驱动 [v2.0.0 新增]
// ============================================================================
//
// 基于 DatabaseDriverBase 模板基类实现 PostgreSQL 专用驱动。
// 使用 Qt 的 QPSQL 驱动，提供 PostgreSQL 特有的功能支持。
//
// 核心设计:
//   - 继承 DatabaseDriverBase<PostgresDriver>，复用公共 SQL 执行逻辑
//   - 实现 open() 和 getType() 钩子方法
//   - 支持 PostgreSQL 特有功能：tableExists() 通过 pg_catalog 查询
//   - executeMigrate() 直接委托给 executeUpdate()
//
// 用法:
//   PostgresDriver driver;
//   ConnectionConfig config;
//   config.host = "localhost";
//   config.port = 5432;
//   config.database = "mydb";
//   config.username = "postgres";
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
// PostgresDriver — PostgreSQL 数据库驱动
// ============================================================================
///
/// @brief PostgreSQL 数据库驱动实现
///
/// 使用 Qt QPSQL 驱动连接 PostgreSQL 数据库。
/// 通过 DatabaseDriverBase 模板基类复用公共 SQL 执行逻辑。
///
/// @par 连接配置
///   - host:     数据库主机地址
///   - port:     端口（默认 5432）
///   - database: 数据库名
///   - username: 用户名
///   - password: 密码
///
/// @thread_safety 非线程安全 — 单线程使用
class PostgresDriver : public DatabaseDriverBase<PostgresDriver> {
public:
    PostgresDriver() = default;
    ~PostgresDriver() override = default;

    // ========================================================================
    // 连接管理
    // ========================================================================

    /// @brief 打开 PostgreSQL 数据库连接
    /// @param config 连接配置
    /// @return 成功返回 Ok，失败返回错误信息
    Result<void> open(const ConnectionConfig& config) override {
        if (isConnected()) {
            (void)close();
        }

        generateConnectionId();
        QString dbName = QString::fromStdString(m_connectionId);

        m_db = QSqlDatabase::addDatabase("QPSQL", dbName);
        m_db.setHostName(config.host);
        m_db.setPort(config.port > 0 ? config.port : 5432);
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
    DatabaseType getType() const override { return DatabaseType::PostgreSQL; }

    // ========================================================================
    // PostgreSQL 特有功能
    // ========================================================================

    /// @brief 检查表是否存在
    /// @param tableName 表名
    /// @return true=表存在
    ///
    /// @note 通过查询 pg_catalog.pg_tables 系统表实现
    Result<bool> tableExists(const QString& tableName) override {
        if (!isConnected()) {
            return Error(ErrorCode::DatabaseError, "Not connected");
        }

        QString sql = QStringLiteral(
            "SELECT EXISTS ("
            "  SELECT 1 FROM pg_catalog.pg_tables "
            "  WHERE schemaname NOT IN ('pg_catalog', 'information_schema') "
            "    AND tablename = ?"
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

#endif // SOUL_DATA_POSTGRES_DRIVER_H