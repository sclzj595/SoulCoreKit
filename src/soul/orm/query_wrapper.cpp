#include <functional>
#include <QDateTime>
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
    Condition cond;
    cond.column = column;
    cond.op = SqlKeyword::IN;
    cond.inValues = values;
    if (!values.empty()) {
        cond.value = values.front();
    }
    m_conditions.push_back(cond);
    return *this;
}

QueryWrapper& QueryWrapper::notIn(const QString& column, const std::vector<QVariant>& values) {
    Condition cond;
    cond.column = column;
    cond.op = SqlKeyword::NOT_IN;
    cond.inValues = values;
    if (!values.empty()) {
        cond.value = values.front();
    }
    m_conditions.push_back(cond);
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

QueryWrapper& QueryWrapper::and_(const std::function<void(QueryWrapper&)>& func) {
    QueryWrapper sub;
    func(sub);
    if (!sub.m_conditions.empty()) {
        if (sub.m_conditions.size() > 1) {
            sub.m_conditions.front().openParens += 1;
            sub.m_conditions.back().closeParens += 1;
        }
        for (auto& cond : sub.m_conditions) {
            cond.logic = SqlKeyword::AND;
            m_conditions.push_back(std::move(cond));
        }
    }
    return *this;
}

QueryWrapper& QueryWrapper::or_(const std::function<void(QueryWrapper&)>& func) {
    QueryWrapper sub;
    func(sub);
    if (!sub.m_conditions.empty()) {
        if (sub.m_conditions.size() > 1) {
            sub.m_conditions.front().openParens += 1;
            sub.m_conditions.back().closeParens += 1;
        }
        sub.m_conditions.front().logic = SqlKeyword::OR;
        for (auto& cond : sub.m_conditions) {
            m_conditions.push_back(std::move(cond));
        }
    }
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

QString QueryWrapper::placeholder(int index) const {
    if (m_dialect) {
        return m_dialect->convertPlaceholder(index);
    }
    return QStringLiteral("?");
}

QString QueryWrapper::buildSelectSql(const QString& tableName) const {
    static const SoftDeleteConfig defaultSd;
    const SoftDeleteConfig* sd = m_dialect ? &m_dialect->softDeleteConfig() : &defaultSd;

    QString sql = "SELECT * FROM " + tableName;
    if (sd->enabled) {
        sql += " WHERE " + sd->columnName + " = " + sd->logicNotDeletedValue;
        appendConditions(sql);
    } else {
        appendConditions(sql);
    }

    if (!m_groupBy.empty()) {
        sql += " GROUP BY " + m_groupBy.join(", ");
    }

    if (!m_orderBy.empty()) {
        sql += " ORDER BY " + m_orderBy.join(", ");
    }

    if (m_limit > 0 || m_offset > 0) {
        QString limitOffset;
        if (m_dialect) {
            limitOffset = m_dialect->buildLimitOffset(m_limit, m_offset);
        } else {
            if (m_limit > 0 && m_offset > 0) {
                limitOffset = QStringLiteral("LIMIT %1 OFFSET %2").arg(m_limit).arg(m_offset);
            } else if (m_limit > 0) {
                limitOffset = QStringLiteral("LIMIT %1").arg(m_limit);
            } else {
                limitOffset = QStringLiteral("OFFSET %1").arg(m_offset);
            }
        }
        sql += " " + limitOffset;
    }

    return sql;
}

QString QueryWrapper::buildCountSql(const QString& tableName) const {
    static const SoftDeleteConfig defaultSd;
    const SoftDeleteConfig* sd = m_dialect ? &m_dialect->softDeleteConfig() : &defaultSd;

    QString sql = "SELECT COUNT(*) FROM " + tableName;
    if (sd->enabled) {
        sql += " WHERE " + sd->columnName + " = " + sd->logicNotDeletedValue;
        appendConditions(sql);
    } else {
        appendConditions(sql);
    }
    return sql;
}

QString QueryWrapper::buildDeleteSql(const QString& tableName) const {
    static const SoftDeleteConfig defaultSd;
    const SoftDeleteConfig* sd = m_dialect ? &m_dialect->softDeleteConfig() : &defaultSd;

    QString sql;
    if (sd->enabled) {
        sql = "UPDATE " + tableName + " SET " + sd->columnName + " = " +
              sd->logicDeletedValue + " WHERE " + sd->columnName + " = " + sd->logicNotDeletedValue;
    } else {
        sql = "DELETE FROM " + tableName;
    }
    appendConditions(sql);
    return sql;
}

QString QueryWrapper::buildUpdateSql(const QString& tableName, const std::map<QString, QVariant>& updates) const {
    static const SoftDeleteConfig defaultSd;
    const SoftDeleteConfig* sd = m_dialect ? &m_dialect->softDeleteConfig() : &defaultSd;

    QString sql = "UPDATE " + tableName + " SET ";
    QStringList setParts;
    int paramIndex = 1;
    for (const auto& pair : updates) {
        setParts << pair.first + " = " + placeholder(paramIndex++);
    }
    sql += setParts.join(", ");
    sql += ", update_time = " + placeholder(paramIndex++);
    if (sd->enabled) {
        sql += " WHERE " + sd->columnName + " = " + sd->logicNotDeletedValue;
    }
    appendConditions(sql, paramIndex);
    return sql;
}

void QueryWrapper::appendConditions(QString& sql, int startIndex) const {
    if (m_conditions.empty()) return;

    bool hasOr = false;
    for (const auto& cond : m_conditions) {
        if (cond.logic == SqlKeyword::OR) {
            hasOr = true;
            break;
        }
    }

    if (hasOr) {
        sql += " AND (";
    } else {
        sql += " AND ";
    }

    int placeholderIndex = startIndex;
    for (size_t i = 0; i < m_conditions.size(); ++i) {
        const auto& cond = m_conditions[i];

        if (i == 0) {
            for (int p = 0; p < cond.openParens; ++p) sql += "(";
            sql += cond.column + " " + opToString(cond.op) + buildValueClause(cond, placeholderIndex);
        } else {
            QString logicStr = (cond.logic == SqlKeyword::OR) ? " OR " : " AND ";
            sql += logicStr;
            for (int p = 0; p < cond.openParens; ++p) sql += "(";
            sql += cond.column + " " + opToString(cond.op) + buildValueClause(cond, placeholderIndex);
        }

        for (int p = 0; p < cond.closeParens; ++p) sql += ")";
    }

    if (hasOr) {
        sql += ")";
    }
}

QString QueryWrapper::buildValueClause(const Condition& cond, int& index) const {
    if (cond.op == SqlKeyword::IS_NULL || cond.op == SqlKeyword::IS_NOT_NULL) {
        return QString();
    }
    if (cond.op == SqlKeyword::IN || cond.op == SqlKeyword::NOT_IN) {
        QStringList placeholders;
        int count = cond.inValues.empty() ? 1 : static_cast<int>(cond.inValues.size());
        for (int i = 0; i < count; ++i) {
            placeholders << placeholder(index++);
        }
        return "(" + placeholders.join(",") + ")";
    }
    return placeholder(index++);
}

std::vector<QVariant> QueryWrapper::getBindValues() const {
    std::vector<QVariant> values;
    for (const auto& cond : m_conditions) {
        if (cond.op == SqlKeyword::IS_NULL || cond.op == SqlKeyword::IS_NOT_NULL) {
            continue;
        }
        if (cond.op == SqlKeyword::IN || cond.op == SqlKeyword::NOT_IN) {
            if (!cond.inValues.empty()) {
                values.insert(values.end(), cond.inValues.begin(), cond.inValues.end());
            } else {
                values.push_back(cond.value);
            }
        } else {
            values.push_back(cond.value);
        }
    }
    return values;
}

std::vector<QVariant> QueryWrapper::getUpdateBindValues(const std::map<QString, QVariant>& updates) const {
    std::vector<QVariant> values;
    for (const auto& pair : updates) {
        values.push_back(pair.second);
    }
    values.push_back(QDateTime::currentDateTime().toString(Qt::ISODate));
    auto condValues = getBindValues();
    values.insert(values.end(), condValues.begin(), condValues.end());
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
