#ifndef SOUL_ORM_ENTITY_H
#define SOUL_ORM_ENTITY_H

#include <QString>
#include <QVariant>
#include <QDateTime>
#include <map>
#include <QObject>

#define TABLE(name) static QString TABLE_NAME() { return QStringLiteral(#name); }
#define ID QString id;
#define COLUMN(name, type) type name;

namespace sc {
namespace orm {

struct FieldMeta {
    QString name;
    QString columnName;
    QString type;
    bool isPrimaryKey = false;
    bool isAutoIncrement = false;
    bool isNullable = true;
    QString defaultValue;
};

struct TableMeta {
    QString tableName;
    std::map<QString, FieldMeta> fields;
    QString primaryKey = "id";
};

class BaseEntity {
public:
    virtual ~BaseEntity() = default;
    QString id;
    QDateTime createTime;
    QDateTime updateTime;
    int deleted = 0;

    virtual TableMeta getTableMeta() const = 0;

    virtual void beforeInsert() {
        createTime = QDateTime::currentDateTime();
        updateTime = QDateTime::currentDateTime();
    }

    virtual void beforeUpdate() {
        updateTime = QDateTime::currentDateTime();
    }

    virtual QVariant getProperty(const QString& name) const {
        Q_UNUSED(name);
        return QVariant();
    }

    virtual void setProperty(const QString& name, const QVariant& value) {
        Q_UNUSED(name);
        Q_UNUSED(value);
    }
};

template<typename Derived>
class Entity : public BaseEntity {
public:
    QVariant getProperty(const QString& name) const override {
        const Derived* derived = static_cast<const Derived*>(this);
        return derived->getPropertyImpl(name);
    }

    void setProperty(const QString& name, const QVariant& value) override {
        Derived* derived = static_cast<Derived*>(this);
        derived->setPropertyImpl(name, value);
    }
};

} // namespace orm
} // namespace sc

#endif