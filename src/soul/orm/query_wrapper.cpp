#include "soul/orm/query_wrapper.h"

namespace sc {
namespace orm {

QueryWrapper::QueryWrapper() {
}

QueryWrapper& QueryWrapper::eq(const QString& column, const QVariant& value) {
    m_conditions.push_back({column, SqlKeyword::EQ, value});
    return *this;
}


QueryWrapper& QueryWrapper::ne(const QString& column, const QVariant& value) {
    m_conditions.push_back({column, SqlKeyword::NE, value});
    return *this;
}

QueryWrapper& QueryWrapper::gt(const QString& column, const QVariant& value) {
    m_conditions.push_back({column, SqlKeyword::GT, value});
    return *this;
}

QueryWrapper& QueryWrapper::ge(const QString& column, const QVariant& value) {
    m_conditions.push_back({column, SqlKeyword::GE, value});
    return *this;
}

QueryWrapper& QueryWrapper::lt(const QString& column, const QVariant& value) {
    m_conditions.push_back({column, SqlKeyword::LT, value});
    return *this;
}

QueryWrapper& QueryWrapper::le(const QString& column, const QVariant& value) {
    m_conditions.push_back({column, SqlKeyword::LE, value});
    return *this;
}

QueryWrapper& QueryWrapper::like(const QString& column, const QString& value) {
    m_conditions.push_back({column, SqlKeyword::LIKE, "%" + value + "%"});
    return *this;
}

QueryWrapper& QueryWrapper::notLike(const QString& column, const QString& value) {
    m_conditions.push_back({column, SqlKeyword::NOT_LIKE, "%" + value + "%"});
    return *this;
}

QueryWrapper& QueryWrapper::likeLeft(const QString& column, const QString& value) {
    m_conditions.push_back({column, SqlKeyword::LIKE, "%" + value});
    return *this;
}

QueryWrapper& QueryWrapper::likeRight(const QString& column, const QString& value) {
    m_conditions.push_back({column, SqlKeyword::LIKE, value + "%"});
    return *this;
}

QueryWrapper& QueryWrapper::in(const QString& column, const std::vector<QVariant>& values) {
    QStringList placeholders;
    for (size_t i = 0; i < values.size(); ++i) {
        placeholders << "?";
    }
    m_conditions.push_back({column, SqlKeyword::IN, placeholders.join(",")});
    return *this;
}

QueryWrapper& QueryWrapper::notIn(const QString& column, const std::vector<QVariant>& values) {
    QStringList placeholders;
    for (size_t i = 0; i < values.size(); ++i) {
        placeholders << "?";
    }
    m_conditions.push_back({column, SqlKeyword::NOT_IN, placeholders.join(",")});
    return *this;
}

QueryWrapper& QueryWrapper::isNull(const QString& column) {
    m_conditions.push_back({column, SqlKeyword::IS_NULL, QVariant()});
    return *this;
}

QueryWrapper& QueryWrapper::isNotNull(const QString& column) {
    m_conditions.push_back({column, SqlKeyword::IS_NOT_NULL, QVariant()});
    return *this;
}

QueryWrapper& QueryWrapper::and_(const std::function<void(QueryWrapper&)>&) {
    return *this;
}

QueryWrapper& QueryWrapper::or_(const std::function<void(QueryWrapper&)>&) {
    return *this;
}

QueryWrapper& QueryWrapper::orderBy(const QString& column, bool asc) {
    m_orderBy.push_back(column + (asc ? " ASC" : " DESC"));
    return *this;
}

QueryWrapper& QueryWrapper::groupBy(const QString& column) {
    m_groupBy.push_back(column);
    return *this;
}

QueryWrapper& QueryWrapper::limit(int size) {
    m_limit = size;
    return *this;
}

QueryWrapper& QueryWrapper::offset(int offset) {
    m_offset = offset;
    return *this;
}

QString QueryWrapper::buildSelectSql(const QString& tableName) const {
    QString sql = "SELECT * FROM " + tableName + " WHERE deleted = 0";

    if (!m_conditions.empty()) {
        for (const auto& cond : m_conditions) {
            sql += " AND " + cond.column + " " + opToString(cond.op);
            if (cond.op != SqlKeyword::IS_NULL && cond.op != SqlKeyword::IS_NOT_NULL) {
                sql += " ?";
            }
        }
    }

    if (!m_groupBy.empty()) {
        QStringList groupByList;
        for (const auto& item : m_groupBy) {
            groupByList << item;
        }
        sql += " GROUP BY " + groupByList.join(", ");
    }

    if (!m_orderBy.empty()) {
        QStringList orderByList;
        for (const auto& item : m_orderBy) {
            orderByList << item;
        }
        sql += " ORDER BY " + orderByList.join(", ");
    }

    if (m_limit > 0) {
        sql += " LIMIT " + QString::number(m_limit);
    }

    if (m_offset > 0) {
        sql += " OFFSET " + QString::number(m_offset);
    }

    return sql;
}

QString QueryWrapper::buildCountSql(const QString& tableName) const {
    QString sql = "SELECT COUNT(*) FROM " + tableName + " WHERE deleted = 0";

    if (!m_conditions.empty()) {
        for (const auto& cond : m_conditions) {
            sql += " AND " + cond.column + " " + opToString(cond.op);
            if (cond.op != SqlKeyword::IS_NULL && cond.op != SqlKeyword::IS_NOT_NULL) {
                sql += " ?";
            }
        }
    }

    return sql;
}

QString QueryWrapper::buildDeleteSql(const QString& tableName) const {
    QString sql = "UPDATE " + tableName + " SET deleted = 1 WHERE deleted = 0";

    if (!m_conditions.empty()) {
        for (const auto& cond : m_conditions) {
            sql += " AND " + cond.column + " " + opToString(cond.op);
            if (cond.op != SqlKeyword::IS_NULL && cond.op != SqlKeyword::IS_NOT_NULL) {
                sql += " ?";
            }
        }
    }

    return sql;
}

QString QueryWrapper::buildUpdateSql(const QString& tableName, const std::map<QString, QVariant>& updates) const {
    QString sql = "UPDATE " + tableName + " SET ";
    QStringList setParts;
    for (const auto& pair : updates) {
        setParts << pair.first + " = ?";
    }
    sql += setParts.join(", ");
    sql += ", update_time = ?";
    sql += " WHERE deleted = 0";

    if (!m_conditions.empty()) {
        for (const auto& cond : m_conditions) {
            sql += " AND " + cond.column + " " + opToString(cond.op);
            if (cond.op != SqlKeyword::IS_NULL && cond.op != SqlKeyword::IS_NOT_NULL) {
                sql += " ?";
            }
        }
    }

    return sql;
}

std::vector<QVariant> QueryWrapper::getBindValues() const {
    std::vector<QVariant> values;
    for (const auto& cond : m_conditions) {
        if (cond.op != SqlKeyword::IS_NULL && cond.op != SqlKeyword::IS_NOT_NULL) {
            values.push_back(cond.value);
        }
    }
    return values;
}

QString QueryWrapper::opToString(SqlKeyword op) const {
    switch (op) {
    case SqlKeyword::EQ: return "=";
    case SqlKeyword::NE: return "<>";
    case SqlKeyword::GT: return ">";
    case SqlKeyword::GE: return ">=";
    case SqlKeyword::LT: return "<";
    case SqlKeyword::LE: return "<=";
    case SqlKeyword::LIKE: return "LIKE";
    case SqlKeyword::NOT_LIKE: return "NOT LIKE";
    case SqlKeyword::IN: return "IN";
    case SqlKeyword::NOT_IN: return "NOT IN";
    case SqlKeyword::IS_NULL: return "IS NULL";
    case SqlKeyword::IS_NOT_NULL: return "IS NOT NULL";
    default: return "=";
    }
}

QString QueryWrapper::valueToString(const QVariant& value) const {
    return value.toString();
}

} // namespace orm
} // namespace sc
