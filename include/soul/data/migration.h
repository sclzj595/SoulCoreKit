#ifndef SOUL_DATA_MIGRATION_H
#define SOUL_DATA_MIGRATION_H

// ============================================================================
// migration.h — 数据库迁移系统 [v2.0.0 新增]
// ============================================================================
//
// 对标 Flyway，提供数据库 schema 版本演进管理。
// 每个迁移包含 up（升级）和 down（回滚）两个方向的 SQL，
// MigrationManager 负责按版本号顺序执行迁移并记录到版本表。
//
// 核心设计:
//   - Migration 结构体: version(版本号), description(描述), up(升级 SQL), down(回滚 SQL)
//   - MigrationManager 类:
//       - registerMigration(Migration) 注册迁移
//       - migrate(driver) 执行所有未应用的迁移
//       - rollback(driver, version) 回滚到指定版本
//       - currentVersion() 获取当前版本
//   - 版本表: _soul_migrations (version, description, applied_at)
//
// 用法:
//   MigrationManager mgr;
//   mgr.registerMigration({"001", "Create users table",
//       "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)",
//       "DROP TABLE users"});
//   mgr.registerMigration({"002", "Add email column",
//       "ALTER TABLE users ADD COLUMN email TEXT",
//       "ALTER TABLE users DROP COLUMN email"});
//   auto result = mgr.migrate(driver);

#include <QString>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <algorithm>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include "soul/core/result.h"
#include "soul/core/error.h"
#include "soul/data/database_driver.h"

namespace sc {
namespace data {

// ============================================================================
// Migration — 单个迁移定义
// ============================================================================
///
/// @brief 数据库迁移定义
///
/// 包含版本号、描述、升级 SQL 和回滚 SQL。
/// 每个迁移对应一次 schema 变更。
///
/// @par 设计原则
///   - 不可变性：迁移一旦发布不应修改，新增变更通过新迁移实现
///   - 幂等性：up/down SQL 应可重复执行
///   - 原子性：每个迁移在独立事务中执行
struct Migration {
    QString version;           ///< 版本号（唯一标识，如 "001", "v2.0.0_001"）
    QString description;       ///< 迁移描述（人类可读）
    QString upSql;             ///< 升级 SQL（向前迁移）
    QString downSql;           ///< 回滚 SQL（向后迁移）

    Migration() = default;
    Migration(QString ver, QString desc, QString up, QString down)
        : version(std::move(ver))
        , description(std::move(desc))
        , upSql(std::move(up))
        , downSql(std::move(down)) {}
};

// ============================================================================
// MigrationRecord — 已应用迁移记录
// ============================================================================
///
/// @brief 已应用迁移的记录（对应 _soul_migrations 表的一行）
struct MigrationRecord {
    QString version;           ///< 版本号
    QString description;       ///< 迁移描述
    QDateTime appliedAt;       ///< 应用时间（UTC）
};

// ============================================================================
// MigrationManager — 迁移管理器
// ============================================================================
///
/// @brief 数据库迁移管理器
///
/// 管理数据库 schema 版本演进，负责：
/// - 跟踪已应用的迁移（_soul_migrations 表）
/// - 按版本号顺序执行待应用迁移
/// - 支持回滚到指定版本
/// - 每个迁移在独立事务中执行，失败自动回滚
///
/// @par _soul_migrations 表结构
/// @code
/// CREATE TABLE _soul_migrations (
///     version     TEXT PRIMARY KEY,
///     description TEXT NOT NULL,
///     applied_at  TEXT NOT NULL
/// );
/// @endcode
///
/// @par 使用示例
/// @code
/// MigrationManager mgr;
/// mgr.registerMigration({"001", "Create users table",
///     "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)",
///     "DROP TABLE users"});
/// mgr.registerMigration({"002", "Add email column",
///     "ALTER TABLE users ADD COLUMN email TEXT",
///     "ALTER TABLE users DROP COLUMN email"});
///
/// auto result = mgr.migrate(driver);
/// if (!result.isOk()) {
///     // 处理错误
/// }
/// @endcode
///
/// @thread_safety Thread-Safe — 内部加锁保护迁移操作
class MigrationManager {
public:
    /// @brief 默认构造函数
    MigrationManager() = default;

    /// @brief 版本表名
    static constexpr const char* kTableName = "_soul_migrations";

    // ========================================================================
    // 注册迁移
    // ========================================================================

    /// @brief 注册迁移
    /// @param migration 迁移定义（非空）
    /// @note 迁移按版本号升序执行；重复注册同一版本将被忽略
    void registerMigration(const Migration& migration) {
        std::lock_guard<std::mutex> lock(m_mutex);
        // 检查是否已存在同版本
        auto it = std::find_if(m_migrations.begin(), m_migrations.end(),
            [&](const Migration& m) { return m.version == migration.version; });
        if (it == m_migrations.end()) {
            m_migrations.push_back(migration);
        }
    }

    /// @brief 注册迁移（移动语义）
    void registerMigration(Migration&& migration) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::find_if(m_migrations.begin(), m_migrations.end(),
            [&](const Migration& m) { return m.version == migration.version; });
        if (it == m_migrations.end()) {
            m_migrations.push_back(std::move(migration));
        }
    }

    // ========================================================================
    // migrate — 执行所有未应用迁移
    // ========================================================================

    /// @brief 执行所有未应用的迁移
    /// @param driver 数据库驱动
    /// @return 成功返回 Ok；失败返回错误（已应用的迁移保持不变）
    ///
    /// @par 执行流程
    /// 1. 确保 _soul_migrations 表存在
    /// 2. 查询已应用的迁移版本
    /// 3. 按版本号排序待应用迁移
    /// 4. 对每个迁移：开事务 → executeUpdate(upSql) → 记录 → 提交（失败则回滚）
    Result<void> migrate(IDatabaseDriver& driver) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 1. 确保版本表存在
        auto ensureResult = ensureMigrationsTable(driver);
        if (!ensureResult.isOk()) {
            return ensureResult;
        }

        // 2. 获取已应用的版本
        auto appliedResult = appliedVersions(driver);
        if (!appliedResult.isOk()) {
            return Error(ErrorCode::DatabaseError, "Failed to query applied migrations");
        }
        auto& applied = appliedResult.unwrap();

        // 3. 按版本号排序(对局部副本排序,不修改成员变量)
        auto sorted = m_migrations;
        std::sort(sorted.begin(), sorted.end(),
            [](const Migration& a, const Migration& b) {
                return a.version < b.version;
            });

        // 4. 逐条执行待应用迁移
        for (const auto& migration : sorted) {
            if (std::find(applied.begin(), applied.end(), migration.version) != applied.end()) {
                continue;  // 已应用，跳过
            }

            // 开事务 → 执行 upSql → 记录 → 提交
            auto txResult = driver.beginTransaction();
            if (!txResult.isOk()) {
                return Error(ErrorCode::DatabaseError,
                    "Failed to begin transaction for migration: " + migration.version);
            }

            auto execResult = driver.executeUpdate(
                migration.upSql, {});
            if (!execResult.isOk()) {
                (void)driver.rollback();
                return Error(ErrorCode::QueryFailed,
                    "Migration failed [" + migration.version + "]: " + driver.getLastError());
            }

            auto recordResult = recordMigration(driver, migration.version, migration.description);
            if (!recordResult.isOk()) {
                (void)driver.rollback();
                return recordResult;
            }

            auto commitResult = driver.commit();
            if (!commitResult.isOk()) {
                return Error(ErrorCode::DatabaseError,
                    "Failed to commit migration: " + migration.version);
            }
        }

        return Ok();
    }

    // ========================================================================
    // rollback — 回滚到指定版本
    // ========================================================================

    /// @brief 回滚到指定版本（不含该版本）
    /// @param driver 数据库驱动
    /// @param targetVersion 目标版本号（该版本本身保持已应用状态）
    /// @return 成功返回 Ok；失败返回错误
    ///
    /// @note 按已应用迁移的逆序回滚，直到目标版本
    Result<void> rollback(IDatabaseDriver& driver, const QString& targetVersion) {
        std::lock_guard<std::mutex> lock(m_mutex);

        // 获取已应用的版本（按应用时间升序）
        auto recordsResult = appliedMigrations(driver);
        if (!recordsResult.isOk()) {
            return Error(ErrorCode::DatabaseError, "Failed to query applied migrations");
        }
        auto& records = recordsResult.unwrap();

        // 按应用时间逆序回滚
        for (auto rit = records.rbegin(); rit != records.rend(); ++rit) {
            if (rit->version <= targetVersion) {
                break;  // 已到达目标版本（不含该版本），停止
            }

            // 查找对应的迁移定义
            auto migIt = std::find_if(m_migrations.begin(), m_migrations.end(),
                [&](const Migration& m) { return m.version == rit->version; });
            if (migIt == m_migrations.end()) {
                return Error(ErrorCode::NotFound,
                    "Migration definition not found for version: " + rit->version);
            }

            // 开事务 → 执行 downSql → 删除记录 → 提交
            auto txResult = driver.beginTransaction();
            if (!txResult.isOk()) {
                return Error(ErrorCode::DatabaseError,
                    "Failed to begin transaction for rollback: " + rit->version);
            }

            auto execResult = driver.executeUpdate(migIt->downSql, {});
            if (!execResult.isOk()) {
                (void)driver.rollback();
                return Error(ErrorCode::QueryFailed,
                    "Rollback failed [" + rit->version + "]: " + driver.getLastError());
            }

            auto removeResult = removeMigrationRecord(driver, rit->version);
            if (!removeResult.isOk()) {
                (void)driver.rollback();
                return removeResult;
            }

            auto commitResult = driver.commit();
            if (!commitResult.isOk()) {
                return Error(ErrorCode::DatabaseError,
                    "Failed to commit rollback: " + rit->version);
            }
        }

        return Ok();
    }

    // ========================================================================
    // currentVersion — 获取当前版本
    // ========================================================================

    /// @brief 获取当前数据库版本号
    /// @param driver 数据库驱动
    /// @return 当前已应用的最新版本号；无迁移应用时返回空字符串
    Result<QString> currentVersion(IDatabaseDriver& driver) const {
        auto result = appliedMigrations(driver);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, "Failed to query current version");
        }
        auto& records = result.unwrap();
        if (records.empty()) {
            return QString("");
        }
        return records.back().version;
    }

    /// @brief 查询已注册的迁移数量
    [[nodiscard]] size_t registeredCount() const noexcept {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_migrations.size();
    }

    /// @brief 查询待应用的迁移版本号
    /// @param driver 数据库驱动
    /// @return 按版本号升序排列的待应用迁移版本
    Result<std::vector<QString>> pendingMigrations(IDatabaseDriver& driver) const {
        std::lock_guard<std::mutex> lock(m_mutex);

        auto appliedResult = appliedVersions(driver);
        if (!appliedResult.isOk()) {
            return Error(ErrorCode::DatabaseError, "Failed to query applied versions");
        }
        auto& applied = appliedResult.unwrap();

        std::vector<QString> pending;
        for (const auto& migration : m_migrations) {
            if (std::find(applied.begin(), applied.end(), migration.version) == applied.end()) {
                pending.push_back(migration.version);
            }
        }
        std::sort(pending.begin(), pending.end());
        return pending;
    }

private:
    // ========================================================================
    // 内部辅助方法
    // ========================================================================

    /// @brief 确保版本表存在
    Result<void> ensureMigrationsTable(IDatabaseDriver& driver) const {
        // 检查表是否存在
        auto existsResult = driver.tableExists(kTableName);
        if (existsResult.isOk() && existsResult.unwrap()) {
            return Ok();
        }

        // 创建版本表
        QString createSql = QStringLiteral(
            "CREATE TABLE IF NOT EXISTS %1 ("
            "  version TEXT PRIMARY KEY, "
            "  description TEXT NOT NULL, "
            "  applied_at TEXT NOT NULL"
            ")")
            .arg(kTableName);

        auto createResult = driver.executeUpdate(createSql, {});
        if (!createResult.isOk()) {
            return Error(ErrorCode::DatabaseError,
                "Failed to create migrations table: " + driver.getLastError());
        }
        return Ok();
    }

    /// @brief 获取已应用的版本列表
    Result<std::vector<QString>> appliedVersions(IDatabaseDriver& driver) const {
        auto result = appliedMigrations(driver);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError, "Failed to query applied versions");
        }
        std::vector<QString> versions;
        for (const auto& record : result.unwrap()) {
            versions.push_back(record.version);
        }
        return versions;
    }

    /// @brief 查询已应用的迁移列表
    Result<std::vector<MigrationRecord>> appliedMigrations(IDatabaseDriver& driver) const {
        // 先确保表存在
        auto ensureResult = ensureMigrationsTable(driver);
        if (!ensureResult.isOk()) {
            return Error(ErrorCode::DatabaseError, "Failed to ensure migrations table");
        }

        QString querySql = QStringLiteral(
            "SELECT version, description, applied_at FROM %1 ORDER BY applied_at ASC")
            .arg(kTableName);

        auto queryResult = driver.executeQuery(querySql, {});
        if (!queryResult.isOk()) {
            return Error(ErrorCode::QueryFailed,
                "Failed to query migrations: " + driver.getLastError());
        }

        std::vector<MigrationRecord> records;
        for (const auto& row : queryResult.unwrap().rows) {
            MigrationRecord record;
            auto vIt = row.find("version");
            auto dIt = row.find("description");
            auto aIt = row.find("applied_at");
            if (vIt != row.end()) record.version = vIt->second.toString();
            if (dIt != row.end()) record.description = dIt->second.toString();
            if (aIt != row.end()) record.appliedAt = QDateTime::fromString(aIt->second.toString(), Qt::ISODate);
            records.push_back(std::move(record));
        }
        return records;
    }

    /// @brief 记录已应用的迁移
    Result<void> recordMigration(IDatabaseDriver& driver,
                                  const QString& version,
                                  const QString& description) const {
        QString insertSql = QStringLiteral(
            "INSERT INTO %1 (version, description, applied_at) VALUES (?, ?, ?)")
            .arg(kTableName);

        auto appliedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
        std::vector<QVariant> params = {version, description, appliedAt};

        auto result = driver.executeUpdate(insertSql, params);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError,
                "Failed to record migration: " + driver.getLastError());
        }
        return Ok();
    }

    /// @brief 删除迁移记录
    Result<void> removeMigrationRecord(IDatabaseDriver& driver,
                                        const QString& version) const {
        QString deleteSql = QStringLiteral(
            "DELETE FROM %1 WHERE version = ?").arg(kTableName);
        std::vector<QVariant> params = {version};

        auto result = driver.executeUpdate(deleteSql, params);
        if (!result.isOk()) {
            return Error(ErrorCode::DatabaseError,
                "Failed to remove migration record: " + driver.getLastError());
        }
        return Ok();
    }

    std::vector<Migration> m_migrations;
    mutable std::mutex m_mutex;
};

} // namespace data
} // namespace sc

#endif // SOUL_DATA_MIGRATION_H