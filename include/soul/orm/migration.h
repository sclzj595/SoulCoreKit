#ifndef SOUL_ORM_MIGRATION_H
#define SOUL_ORM_MIGRATION_H

#include <QString>
#include <QDateTime>
#include <atomic>
#include <memory>
#include <vector>
#include <mutex>
#include "soul/core/result.h"
#include "soul/data/database_driver.h"

namespace sc {
namespace orm {
namespace migration {

/**
 * @brief 迁移历史记录
 *
 * 表示一个已执行的迁移，对应 schema_migrations 表的一行。
 */
struct MigrationRecord {
    QString   version;       ///< 版本号（唯一标识，如 "001" 或 "20260726_001"）
    QString   description;   ///< 迁移描述
    QDateTime appliedAt;     ///< 应用时间（UTC）
};

/**
 * @brief 迁移接口
 *
 * 每个迁移必须实现 up（升级）和 down（回滚）两个方向。
 *
 * @par 设计原则
 * - 单一职责：每个迁移只负责一次 schema 变更
 * - 幂等性：up/down 应可重复执行（依赖事务保证原子性）
 * - 不可变：迁移一旦发布不应修改，新增变更通过新迁移实现
 *
 * @thread_safety Implementations should be stateless and thread-safe.
 */
class IMigration {
public:
    virtual ~IMigration() = default;

    /// @return 迁移版本号（唯一标识）
    [[nodiscard]] virtual QString version() const = 0;

    /// @return 迁移描述（人类可读）
    [[nodiscard]] virtual QString description() const = 0;

    /// @brief 升级迁移（向前）
    virtual Result<void> up(data::IDatabaseDriver& driver) const = 0;

    /// @brief 回滚迁移（向后）
    virtual Result<void> down(data::IDatabaseDriver& driver) const = 0;
};

/**
 * @brief 迁移基类（便捷实现）
 *
 * 提供版本号和描述的存储，用户只需实现 up/down。
 *
 * @par 使用示例
 * @code
 * class CreateUsersTable : public BaseMigration {
 * public:
 *     CreateUsersTable() : BaseMigration("001", "Create users table") {}
 *
 *     Result<void> up(IDatabaseDriver& driver) const override {
 *         return driver.executeUpdate(
 *             "CREATE TABLE users (id TEXT PRIMARY KEY, name TEXT)"
 *         ).isOk() ? Ok() : Result<void>(Error{});
 *     }
 *
 *     Result<void> down(IDatabaseDriver& driver) const override {
 *         return driver.executeUpdate("DROP TABLE users").isOk()
 *              ? Ok() : Result<void>(Error{});
 *     }
 * };
 * @endcode
 */
class BaseMigration : public IMigration {
public:
    BaseMigration(QString version, QString description)
        : m_version(std::move(version))
        , m_description(std::move(description)) {}

    [[nodiscard]] QString version() const override { return m_version; }
    [[nodiscard]] QString description() const override { return m_description; }

private:
    QString m_version;
    QString m_description;
};

/**
 * @brief 迁移管理器
 *
 * 管理数据库 schema 版本演进，负责：
 * - 跟踪已应用的迁移（schema_migrations 表）
 * - 按版本号顺序执行待应用迁移
 * - 支持回滚到指定版本
 * - 每个迁移在独立事务中执行，失败自动回滚
 *
 * @par schema_migrations 表结构
 * @code
 * CREATE TABLE schema_migrations (
 *     version     TEXT PRIMARY KEY,
 *     description TEXT NOT NULL,
 *     applied_at  TEXT NOT NULL
 * );
 * @endcode
 *
 * @par 使用示例
 * @code
 * auto driver = DatabaseDriverFactory::instance().create(DatabaseType::SQLite);
 * driver->open(config);
 *
 * MigrationManager mgr(driver);
 * mgr.addMigration(std::make_shared<CreateUsersTable>());
 * mgr.addMigration(std::make_shared<AddEmailColumn>());
 *
 * auto result = mgr.migrate();  // 应用所有待执行迁移
 * if (!result.isOk()) {
 *     // 处理错误
 * }
 * @endcode
 *
 * @thread_safety Thread-Safe — 内部加锁保护迁移操作
 */
class MigrationManager {
public:
    /**
     * @brief 构造函数
     * @param driver 数据库驱动（非空，生命周期由调用方管理）
     * @throws std::invalid_argument 若 driver 为空
     */
    explicit MigrationManager(std::shared_ptr<data::IDatabaseDriver> driver);

    /**
     * @brief 注册迁移
     * @param migration 迁移实例（非空）
     * @note 迁移按版本号升序执行；重复注册同一版本将忽略
     */
    void addMigration(std::shared_ptr<IMigration> migration);

    /**
     * @brief 应用所有待执行迁移
     * @return 成功返回 Ok；失败返回错误（已应用的迁移保持不变）
     *
     * @par 执行流程
     * 1. 确保 schema_migrations 表存在
     * 2. 查询已应用的迁移版本
     * 3. 按版本号排序待应用迁移
     * 4. 对每个迁移：开事务 → up() → 记录 → 提交（失败则回滚）
     */
    [[nodiscard]] Result<void> migrate();

    /**
     * @brief 回滚指定数量的迁移
     * @param steps 回滚步数（默认 1）
     * @return 成功返回 Ok；失败返回错误
     *
     * @note 按已应用迁移的逆序回滚
     */
    [[nodiscard]] Result<void> rollback(int steps = 1);

    /**
     * @brief 回滚到指定版本（不含该版本）
     * @param targetVersion 目标版本（该版本本身保持已应用状态）
     * @return 成功返回 Ok；失败返回错误
     */
    [[nodiscard]] Result<void> rollbackTo(const QString& targetVersion);

    /**
     * @brief 查询已应用的迁移列表
     * @return 按应用时间升序排列的迁移记录
     */
    [[nodiscard]] Result<std::vector<MigrationRecord>> appliedMigrations() const;

    /**
     * @brief 查询待应用的迁移版本号
     * @return 按版本号升序排列的待应用迁移版本
     */
    [[nodiscard]] Result<std::vector<QString>> pendingMigrations() const;

    /**
     * @brief 查询当前最新版本号
     * @return 当前已应用的最新版本号；无迁移应用时返回空字符串
     */
    [[nodiscard]] Result<QString> currentVersion() const;

    /**
     * @brief 查询已注册的迁移数量
     */
    [[nodiscard]] std::size_t registeredCount() const noexcept;

    /**
     * @brief schema_migrations 表名
     */
    static constexpr const char* kTableName = "schema_migrations";

private:
    // 确保迁移记录表存在(const 语义:首次调用可能写入,后续调用无副作用)
    [[nodiscard]] Result<void> ensureMigrationsTable() const;
    // 检查版本是否已应用
    [[nodiscard]] Result<bool> isApplied(const QString& version) const;
    // 记录已应用的迁移
    [[nodiscard]] Result<void> recordMigration(const QString& version,
                                                const QString& description);
    // 删除迁移记录
    [[nodiscard]] Result<void> removeMigrationRecord(const QString& version);
    // 查询已应用版本集合
    [[nodiscard]] Result<std::vector<QString>> appliedVersions() const;
    // 单个迁移执行（事务包裹）
    [[nodiscard]] Result<void> applyMigration(const IMigration& m, bool isUp);

    std::shared_ptr<data::IDatabaseDriver> m_driver;
    std::vector<std::shared_ptr<IMigration>> m_migrations;
    mutable std::mutex m_mutex;
    mutable std::atomic<bool> m_tableEnsured{false};  ///< mutable: const 成员函数可修改
};

} // namespace migration
} // namespace orm
} // namespace sc

#endif // SOUL_ORM_MIGRATION_H
