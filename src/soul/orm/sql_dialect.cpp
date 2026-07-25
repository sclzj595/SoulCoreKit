#include "soul/orm/sql_dialect.h"
#include "soul/core/error.h"
#include <stdexcept>

namespace sc {
namespace orm {

// ---------------------------------------------------------------------------
// SQLiteDialect
// ---------------------------------------------------------------------------
class SQLiteDialect : public ISqlDialect {
public:
    QString getDriverName() const override { return QStringLiteral("QSQLITE"); }
    SqlDialectType getType() const override { return SqlDialectType::SQLite; }

    QString buildLimitOffset(int limit, int offset) const override {
        if (limit > 0 && offset > 0) {
            return QStringLiteral("LIMIT %1 OFFSET %2").arg(limit).arg(offset);
        }
        if (limit > 0) {
            return QStringLiteral("LIMIT %1").arg(limit);
        }
        if (offset > 0) {
            return QStringLiteral("OFFSET %1").arg(offset);
        }
        return {};
    }

    QString getAutoIncrementKeyword() const override { return QStringLiteral("AUTOINCREMENT"); }
    QString getPrimaryKeyKeyword() const override { return QStringLiteral("PRIMARY KEY"); }
    QString escapeIdentifier(const QString& id) const override {
        return QStringLiteral("\"%1\"").arg(id);
    }
    QString escapeString(const QString& str) const override {
        QString escaped = str;
        escaped.replace("'", "''");
        return QStringLiteral("'%1'").arg(escaped);
    }
    QString castToDateTime(const QString& value) const override { return value; }
    QString castToString(const QString& value) const override { return value; }
    QString castToInt(const QString& value) const override { return value; }
    QString castToFloat(const QString& value) const override { return value; }
    QString getCurrentTimestampFunction() const override { return QStringLiteral("CURRENT_TIMESTAMP"); }
    QString getNowFunction() const override { return QStringLiteral("datetime('now')"); }
    QString getConcatFunction(const std::vector<QString>& args) const override {
        if (args.empty()) return QStringLiteral("''");
        QStringList list(args.begin(), args.end());
        return list.join(" || ");
    }
    QString getLikeEscapeClause() const override { return QStringLiteral("ESCAPE '\\'"); }
    QString getCreateTableSuffix() const override { return {}; }
    QString getDropTableIfExists(const QString& tableName) const override {
        return QStringLiteral("DROP TABLE IF EXISTS %1").arg(tableName);
    }
    QString convertPlaceholder(int index) const override {
        Q_UNUSED(index);
        return QStringLiteral("?");
    }

    const SoftDeleteConfig& softDeleteConfig() const override { return m_softDelete; }
    void setSoftDeleteConfig(SoftDeleteConfig config) override { m_softDelete = std::move(config); }

private:
    SoftDeleteConfig m_softDelete;
};

// ---------------------------------------------------------------------------
// MySqlDialect
// ---------------------------------------------------------------------------
class MySqlDialect : public ISqlDialect {
public:
    QString getDriverName() const override { return QStringLiteral("QMYSQL"); }
    SqlDialectType getType() const override { return SqlDialectType::MySQL; }

    QString buildLimitOffset(int limit, int offset) const override {
        if (limit > 0 && offset > 0) {
            return QStringLiteral("LIMIT %1, %2").arg(offset).arg(limit);
        }
        if (limit > 0) {
            return QStringLiteral("LIMIT %1").arg(limit);
        }
        return {};
    }

    QString getAutoIncrementKeyword() const override { return QStringLiteral("AUTO_INCREMENT"); }
    QString getPrimaryKeyKeyword() const override { return QStringLiteral("PRIMARY KEY"); }
    QString escapeIdentifier(const QString& id) const override {
        return QStringLiteral("`%1`").arg(id);
    }
    QString escapeString(const QString& str) const override {
        QString escaped = str;
        escaped.replace("'", "''");
        escaped.replace("\\", "\\\\");
        return QStringLiteral("'%1'").arg(escaped);
    }
    QString castToDateTime(const QString& value) const override {
        return QStringLiteral("CAST(%1 AS DATETIME)").arg(value);
    }
    QString castToString(const QString& value) const override {
        return QStringLiteral("CAST(%1 AS CHAR)").arg(value);
    }
    QString castToInt(const QString& value) const override {
        return QStringLiteral("CAST(%1 AS SIGNED)").arg(value);
    }
    QString castToFloat(const QString& value) const override {
        return QStringLiteral("CAST(%1 AS DECIMAL(20,6))").arg(value);
    }
    QString getCurrentTimestampFunction() const override { return QStringLiteral("CURRENT_TIMESTAMP"); }
    QString getNowFunction() const override { return QStringLiteral("NOW()"); }
    QString getConcatFunction(const std::vector<QString>& args) const override {
        QStringList list(args.begin(), args.end());
        return QStringLiteral("CONCAT(%1)").arg(list.join(", "));
    }
    QString getLikeEscapeClause() const override { return {}; }
    QString getCreateTableSuffix() const override { return QStringLiteral("ENGINE=InnoDB DEFAULT CHARSET=utf8mb4"); }
    QString getDropTableIfExists(const QString& tableName) const override {
        return QStringLiteral("DROP TABLE IF EXISTS %1").arg(tableName);
    }
    QString convertPlaceholder(int index) const override {
        Q_UNUSED(index);
        return QStringLiteral("?");
    }

    const SoftDeleteConfig& softDeleteConfig() const override { return m_softDelete; }
    void setSoftDeleteConfig(SoftDeleteConfig config) override { m_softDelete = std::move(config); }

private:
    SoftDeleteConfig m_softDelete;
};

// ---------------------------------------------------------------------------
// PostgreSqlDialect
// ---------------------------------------------------------------------------
class PostgreSqlDialect : public ISqlDialect {
public:
    QString getDriverName() const override { return QStringLiteral("QPSQL"); }
    SqlDialectType getType() const override { return SqlDialectType::PostgreSQL; }

    QString buildLimitOffset(int limit, int offset) const override {
        if (limit > 0 && offset > 0) {
            return QStringLiteral("LIMIT %1 OFFSET %2").arg(limit).arg(offset);
        }
        if (limit > 0) {
            return QStringLiteral("LIMIT %1").arg(limit);
        }
        if (offset > 0) {
            return QStringLiteral("OFFSET %1").arg(offset);
        }
        return {};
    }

    QString getAutoIncrementKeyword() const override { return QStringLiteral("SERIAL"); }
    QString getPrimaryKeyKeyword() const override { return QStringLiteral("PRIMARY KEY"); }
    QString escapeIdentifier(const QString& id) const override {
        return QStringLiteral("\"%1\"").arg(id);
    }
    QString escapeString(const QString& str) const override {
        QString escaped = str;
        escaped.replace("'", "''");
        return QStringLiteral("'%1'").arg(escaped);
    }
    QString castToDateTime(const QString& value) const override {
        return QStringLiteral("%1::timestamp").arg(value);
    }
    QString castToString(const QString& value) const override {
        return QStringLiteral("%1::text").arg(value);
    }
    QString castToInt(const QString& value) const override {
        return QStringLiteral("%1::integer").arg(value);
    }
    QString castToFloat(const QString& value) const override {
        return QStringLiteral("%1::double precision").arg(value);
    }
    QString getCurrentTimestampFunction() const override { return QStringLiteral("CURRENT_TIMESTAMP"); }
    QString getNowFunction() const override { return QStringLiteral("NOW()"); }
    QString getConcatFunction(const std::vector<QString>& args) const override {
        if (args.empty()) return QStringLiteral("''");
        QStringList list(args.begin(), args.end());
        return list.join(" || ");
    }
    QString getLikeEscapeClause() const override { return {}; }
    QString getCreateTableSuffix() const override { return {}; }
    QString getDropTableIfExists(const QString& tableName) const override {
        return QStringLiteral("DROP TABLE IF EXISTS %1").arg(tableName);
    }
    QString convertPlaceholder(int index) const override {
        return QStringLiteral("$%1").arg(index);
    }

    const SoftDeleteConfig& softDeleteConfig() const override { return m_softDelete; }
    void setSoftDeleteConfig(SoftDeleteConfig config) override { m_softDelete = std::move(config); }

private:
    SoftDeleteConfig m_softDelete;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
std::unique_ptr<ISqlDialect> ISqlDialect::create(SqlDialectType type) {
    switch (type) {
    case SqlDialectType::SQLite:
        return std::make_unique<SQLiteDialect>();
    case SqlDialectType::MySQL:
        return std::make_unique<MySqlDialect>();
    case SqlDialectType::PostgreSQL:
        return std::make_unique<PostgreSqlDialect>();
    default:
        throw std::invalid_argument("Unsupported SQL dialect type");
    }
}

} // namespace orm
} // namespace sc
