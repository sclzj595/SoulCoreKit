#ifndef SOUL_ORM_QUERY_WRAPPER_H
#define SOUL_ORM_QUERY_WRAPPER_H

#include <QString>
#include <QVariant>
#include <vector>
#include <map>
#include <memory>
#include <functional>

namespace sc {
namespace orm {

enum class SqlKeyword {
    EQ, NE, GT, GE, LT, LE,
    LIKE, NOT_LIKE, LIKE_LEFT, LIKE_RIGHT,
    IN, NOT_IN,
    IS_NULL, IS_NOT_NULL,
    AND, OR
};

struct Condition {
    QString column;
    SqlKeyword op;
    QVariant value;
    SqlKeyword logic = SqlKeyword::AND;
};

class QueryWrapper {
public:
    QueryWrapper();

    QueryWrapper& eq(const QString& column, const QVariant& value);
    QueryWrapper& ne(const QString& column, const QVariant& value);
    QueryWrapper& gt(const QString& column, const QVariant& value);
    QueryWrapper& ge(const QString& column, const QVariant& value);
    QueryWrapper& lt(const QString& column, const QVariant& value);
    QueryWrapper& le(const QString& column, const QVariant& value);

    QueryWrapper& like(const QString& column, const QString& value);
    QueryWrapper& notLike(const QString& column, const QString& value);
    QueryWrapper& likeLeft(const QString& column, const QString& value);
    QueryWrapper& likeRight(const QString& column, const QString& value);

    QueryWrapper& in(const QString& column, const std::vector<QVariant>& values);
    QueryWrapper& notIn(const QString& column, const std::vector<QVariant>& values);

    QueryWrapper& isNull(const QString& column);
    QueryWrapper& isNotNull(const QString& column);

    QueryWrapper& and_(const std::function<void(QueryWrapper&)>& func);
    QueryWrapper& or_(const std::function<void(QueryWrapper&)>& func);

    QueryWrapper& orderBy(const QString& column, bool asc = true);
    QueryWrapper& groupBy(const QString& column);

    QueryWrapper& limit(int size);
    QueryWrapper& offset(int offset);

    QString buildSelectSql(const QString& tableName) const;
    QString buildCountSql(const QString& tableName) const;
    QString buildDeleteSql(const QString& tableName) const;
    QString buildUpdateSql(const QString& tableName, const std::map<QString, QVariant>& updates) const;

    std::vector<QVariant> getBindValues() const;

private:
    std::vector<Condition> m_conditions;
    std::vector<QString> m_orderBy;
    std::vector<QString> m_groupBy;
    int m_limit = -1;
    int m_offset = -1;

    QString opToString(SqlKeyword op) const;
    QString valueToString(const QVariant& value) const;
};

} // namespace orm
} // namespace sc

#endif