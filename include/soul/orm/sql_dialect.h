#ifndef SOUL_ORM_SQL_DIALECT_H
#define SOUL_ORM_SQL_DIALECT_H

#include <QString>
#include <QVariant>
#include <vector>
#include <map>
#include <memory>

namespace sc {
namespace orm {

enum class SqlDialectType { SQLite, MySQL, PostgreSQL, MSSQL, Oracle };

class ISqlDialect {
public:
    virtual ~ISqlDialect() = default;
    virtual QString getDriverName() const = 0;
    virtual SqlDialectType getType() const = 0;
    virtual QString buildLimitOffset(int limit, int offset) const = 0;
    virtual QString getAutoIncrementKeyword() const = 0;
    virtual QString getPrimaryKeyKeyword() const = 0;
    virtual QString escapeIdentifier(const QString& identifier) const = 0;
    virtual QString escapeString(const QString& str) const = 0;
    virtual QString castToDateTime(const QString& value) const = 0;
    virtual QString castToString(const QString& value) const = 0;
    virtual QString castToInt(const QString& value) const = 0;
    virtual QString castToFloat(const QString& value) const = 0;
    virtual QString getCurrentTimestampFunction() const = 0;
    virtual QString getNowFunction() const = 0;
    virtual QString getConcatFunction(const std::vector<QString>& args) const = 0;
    virtual QString getLikeEscapeClause() const = 0;
    virtual QString getCreateTableSuffix() const = 0;
    virtual QString getDropTableIfExists(const QString& tableName) const = 0;
    virtual QString convertPlaceholder(int index) const = 0;
    static std::unique_ptr<ISqlDialect> create(SqlDialectType type);
};

} // namespace orm
} // namespace sc

#endif
