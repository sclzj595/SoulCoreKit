#include "soul/orm/migration.h"

#include <algorithm>
#include <unordered_set>
#include <stdexcept>

namespace sc {
namespace orm {
namespace migration {

namespace {

// 标准比较：版本号按字符串升序（约定版本号格式为可排序的，如 "001"、"20260726_001"）
bool versionLess(const QString& a, const QString& b) {
    return a < b;
}

} // namespace

MigrationManager::MigrationManager(std::shared_ptr<data::IDatabaseDriver> driver)
    : m_driver(std::move(driver)) {
    if (!m_driver) {
        throw std::invalid_argument("MigrationManager: driver must not be null");
    }
}

void MigrationManager::addMigration(std::shared_ptr<IMigration> migration) {
    if (!migration) {
        return;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    // 去重：同一版本号只保留首次注册的迁移
    for (const auto& m : m_migrations) {
        if (m->version() == migration->version()) {
            return;
        }
    }
    m_migrations.push_back(std::move(migration));
}

Result<void> MigrationManager::migrate() {
    if (!m_driver->isConnected()) {
        return Error(ErrorCode::NotConnected,
                     "MigrationManager::migrate: driver not connected");
    }

    // 1. 确保迁移表存在(内部自同步,无需外层持锁)
    auto ensureResult = ensureMigrationsTable();
    if (!ensureResult.isOk()) {
        return ensureResult.unwrapErr();
    }

    // 2. 查询已应用版本(driver 内部线程安全,无需持锁)
    auto appliedResult = appliedVersions();
    if (!appliedResult.isOk()) {
        return appliedResult.unwrapErr();
    }
    auto appliedSet = std::move(appliedResult.unwrap());
    std::unordered_set<QString> appliedLookup(appliedSet.begin(), appliedSet.end());

    // 3. 拷贝待应用迁移快照(仅在访问 m_migrations 时持锁)
    std::vector<std::shared_ptr<IMigration>> pending;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& m : m_migrations) {
            if (appliedLookup.find(m->version()) == appliedLookup.end()) {
                pending.push_back(m);
            }
        }
    }
    std::sort(pending.begin(), pending.end(),
              [](const auto& a, const auto& b) {
                  return versionLess(a->version(), b->version());
              });

    // 4. 逐个应用迁移(driver 调用移出锁外,避免与 addMigration 死锁)
    for (const auto& m : pending) {
        auto r = applyMigration(*m, /*isUp=*/true);
        if (!r.isOk()) {
            return r.unwrapErr();
        }
    }

    return Ok();
}

Result<void> MigrationManager::rollback(int steps) {
    if (steps <= 0) {
        return Ok();
    }

    if (!m_driver->isConnected()) {
        return Error(ErrorCode::NotConnected,
                     "MigrationManager::rollback: driver not connected");
    }

    auto ensureResult = ensureMigrationsTable();
    if (!ensureResult.isOk()) {
        return ensureResult.unwrapErr();
    }

    // 查询已应用版本(driver 内部线程安全,无需持锁)
    auto appliedResult = appliedVersions();
    if (!appliedResult.isOk()) {
        return appliedResult.unwrapErr();
    }
    auto applied = std::move(appliedResult.unwrap());
    std::sort(applied.begin(), applied.end(),
              [](const QString& a, const QString& b) { return a > b; });  // 降序

    // 拷贝迁移定义快照(仅在访问 m_migrations 时持锁)
    std::unordered_map<QString, std::shared_ptr<IMigration>> migrationMap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& m : m_migrations) {
            migrationMap.emplace(m->version(), m);
        }
    }

    // 回滚(driver 调用移出锁外)
    int rolledBack = 0;
    for (const auto& version : applied) {
        if (rolledBack >= steps) {
            break;
        }
        auto it = migrationMap.find(version);
        if (it == migrationMap.end()) {
            // 已应用但未注册迁移定义，无法回滚
            return Error(ErrorCode::InvalidState,
                         QString("Migration %1 applied but not registered").arg(version));
        }
        auto r = applyMigration(*it->second, /*isUp=*/false);
        if (!r.isOk()) {
            return r.unwrapErr();
        }
        ++rolledBack;
    }

    return Ok();
}

Result<void> MigrationManager::rollbackTo(const QString& targetVersion) {
    if (!m_driver->isConnected()) {
        return Error(ErrorCode::NotConnected,
                     "MigrationManager::rollbackTo: driver not connected");
    }

    auto ensureResult = ensureMigrationsTable();
    if (!ensureResult.isOk()) {
        return ensureResult.unwrapErr();
    }

    auto appliedResult = appliedVersions();
    if (!appliedResult.isOk()) {
        return appliedResult.unwrapErr();
    }
    auto applied = std::move(appliedResult.unwrap());
    // 降序排列，回滚所有 > targetVersion 的迁移
    std::sort(applied.begin(), applied.end(),
              [](const QString& a, const QString& b) { return a > b; });

    // 拷贝迁移定义快照
    std::unordered_map<QString, std::shared_ptr<IMigration>> migrationMap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& m : m_migrations) {
            migrationMap.emplace(m->version(), m);
        }
    }

    for (const auto& version : applied) {
        if (version <= targetVersion) {
            break;
        }
        auto it = migrationMap.find(version);
        if (it == migrationMap.end()) {
            return Error(ErrorCode::InvalidState,
                         QString("Migration %1 applied but not registered").arg(version));
        }
        auto r = applyMigration(*it->second, /*isUp=*/false);
        if (!r.isOk()) {
            return r.unwrapErr();
        }
    }

    return Ok();
}

Result<std::vector<MigrationRecord>> MigrationManager::appliedMigrations() const {
    if (!m_driver->isConnected()) {
        return Error(ErrorCode::NotConnected,
                     "MigrationManager::appliedMigrations: driver not connected");
    }

    auto ensureResult = ensureMigrationsTable();
    if (!ensureResult.isOk()) {
        return ensureResult.unwrapErr();
    }

    // driver 调用移出锁外(driver 内部线程安全)
    QString sql = QString("SELECT version, description, applied_at FROM %1 "
                          "ORDER BY applied_at ASC")
                      .arg(QString::fromLatin1(kTableName));
    auto result = m_driver->executeQuery(sql);
    if (!result.isOk()) {
        return result.unwrapErr();
    }

    std::vector<MigrationRecord> records;
    auto queryResult = result.unwrap();
    records.reserve(queryResult.rows.size());
    for (const auto& row : queryResult.rows) {
        MigrationRecord r;
        auto it = row.find("version");
        if (it != row.end()) r.version = it->second.toString();
        it = row.find("description");
        if (it != row.end()) r.description = it->second.toString();
        it = row.find("applied_at");
        if (it != row.end()) r.appliedAt = QDateTime::fromString(it->second.toString(), Qt::ISODateWithMs);
        records.push_back(std::move(r));
    }
    return records;
}

Result<std::vector<QString>> MigrationManager::pendingMigrations() const {
    if (!m_driver->isConnected()) {
        return Error(ErrorCode::NotConnected,
                     "MigrationManager::pendingMigrations: driver not connected");
    }

    auto ensureResult = ensureMigrationsTable();
    if (!ensureResult.isOk()) {
        return ensureResult.unwrapErr();
    }

    // driver 调用移出锁外
    auto appliedResult = appliedVersions();
    if (!appliedResult.isOk()) {
        return appliedResult.unwrapErr();
    }
    auto applied = std::move(appliedResult.unwrap());
    std::unordered_set<QString> appliedLookup(applied.begin(), applied.end());

    // 仅在访问 m_migrations 时持锁
    std::vector<QString> pending;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& m : m_migrations) {
            if (appliedLookup.find(m->version()) == appliedLookup.end()) {
                pending.push_back(m->version());
            }
        }
    }
    std::sort(pending.begin(), pending.end(), versionLess);
    return pending;
}

Result<QString> MigrationManager::currentVersion() const {
    auto result = appliedMigrations();
    if (!result.isOk()) {
        return result.unwrapErr();
    }
    auto records = std::move(result.unwrap());
    if (records.empty()) {
        return QString();
    }
    // 返回最后一个应用的版本
    return records.back().version;
}

std::size_t MigrationManager::registeredCount() const noexcept {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_migrations.size();
}

// ===== 私有方法 =====

Result<void> MigrationManager::ensureMigrationsTable() const {
    // 快速路径:已确保过(无需持锁)
    if (m_tableEnsured.load(std::memory_order_acquire)) {
        return Ok();
    }

    // 慢速路径:CREATE TABLE IF NOT EXISTS 是幂等的,并发执行安全
    QString sql = QString("CREATE TABLE IF NOT EXISTS %1 ("
                          "version     TEXT PRIMARY KEY, "
                          "description TEXT NOT NULL, "
                          "applied_at  TEXT NOT NULL"
                          ")")
                      .arg(QString::fromLatin1(kTableName));
    auto result = m_driver->executeUpdate(sql);
    if (!result.isOk()) {
        return result.unwrapErr();
    }

    m_tableEnsured.store(true, std::memory_order_release);
    return Ok();
}

Result<bool> MigrationManager::isApplied(const QString& version) const {
    QString sql = QString("SELECT version FROM %1 WHERE version = ?")
                      .arg(QString::fromLatin1(kTableName));
    auto result = m_driver->executeQuery(sql, {QVariant(version)});
    if (!result.isOk()) {
        return result.unwrapErr();
    }
    return !result.unwrap().rows.empty();
}

Result<void> MigrationManager::recordMigration(const QString& version,
                                                const QString& description) {
    QString appliedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
    QString sql = QString("INSERT INTO %1 (version, description, applied_at) "
                          "VALUES (?, ?, ?)")
                      .arg(QString::fromLatin1(kTableName));
    auto result = m_driver->executeUpdate(sql,
                                           {QVariant(version),
                                            QVariant(description),
                                            QVariant(appliedAt)});
    if (!result.isOk()) {
        return result.unwrapErr();
    }
    return Ok();
}

Result<void> MigrationManager::removeMigrationRecord(const QString& version) {
    QString sql = QString("DELETE FROM %1 WHERE version = ?")
                      .arg(QString::fromLatin1(kTableName));
    auto result = m_driver->executeUpdate(sql, {QVariant(version)});
    if (!result.isOk()) {
        return result.unwrapErr();
    }
    return Ok();
}

Result<std::vector<QString>> MigrationManager::appliedVersions() const {
    QString sql = QString("SELECT version FROM %1").arg(QString::fromLatin1(kTableName));
    auto result = m_driver->executeQuery(sql);
    if (!result.isOk()) {
        return result.unwrapErr();
    }
    std::vector<QString> versions;
    auto queryResult = result.unwrap();
    versions.reserve(queryResult.rows.size());
    for (const auto& row : queryResult.rows) {
        auto it = row.find("version");
        if (it != row.end()) {
            versions.push_back(it->second.toString());
        }
    }
    return versions;
}

Result<void> MigrationManager::applyMigration(const IMigration& m, bool isUp) {
    // 事务包裹：开事务 → 执行迁移 → 记录/删除 → 提交
    auto beginResult = m_driver->beginTransaction();
    if (!beginResult.isOk()) {
        return beginResult.unwrapErr();
    }

    auto execResult = isUp ? m.up(*m_driver) : m.down(*m_driver);
    if (!execResult.isOk()) {
        (void)m_driver->rollback();
        return execResult.unwrapErr();
    }

    auto recordResult = isUp
        ? recordMigration(m.version(), m.description())
        : removeMigrationRecord(m.version());
    if (!recordResult.isOk()) {
        (void)m_driver->rollback();
        return recordResult.unwrapErr();
    }

    auto commitResult = m_driver->commit();
    if (!commitResult.isOk()) {
        return commitResult.unwrapErr();
    }

    return Ok();
}

} // namespace migration
} // namespace orm
} // namespace sc
