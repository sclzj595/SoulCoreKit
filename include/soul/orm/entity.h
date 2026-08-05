#ifndef SOUL_ORM_ENTITY_H
#define SOUL_ORM_ENTITY_H

#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QtGlobal>
#include <map>

namespace sc {
namespace orm {

struct FieldMeta {
    QString name;
    QString columnName;
    QString typeName;
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

template<typename Derived>
class Entity {
public:
    QString id;
    QDateTime createTime;
    QDateTime updateTime;
    int deleted = 0;

    virtual ~Entity() = default;

    virtual void beforeInsert() {
        createTime = QDateTime::currentDateTime();
        updateTime = QDateTime::currentDateTime();
    }

    virtual void beforeUpdate() {
        updateTime = QDateTime::currentDateTime();
    }

    virtual TableMeta getTableMeta() const {
        return Derived::tableMeta();
    }

    // [v2.5.1] 移除 virtual 关键字：CRTP 通过 static_cast 在编译期分发，
    // virtual 增加 vtable 开销且暗示可被子类重写（与 CRTP 模式矛盾）。
    // 如果 Derived 未实现 getPropertyImpl，编译期报错。
    QVariant getProperty(const QString& name) const {
        const Derived* derived = static_cast<const Derived*>(this);
        return derived->getPropertyImpl(name);
    }

    void setProperty(const QString& name, const QVariant& value) {
        Derived* derived = static_cast<Derived*>(this);
        derived->setPropertyImpl(name, value);
    }
};

#define SC_ENTITY_TABLE(className, tableName) \
    static QString TABLE_NAME() { return QStringLiteral(tableName); }

#define SC_FIELD(type, name) type name;
#define SC_FIELD_N(type, name, columnName) type name;
#define SC_FIELD_NOTNULL(type, name) type name;
#define SC_FIELD_DEFAULT(type, name, defaultValue) type name;

#define SC_TABLE_META(className, tableName) \
    TableMeta className::tableMeta() { \
        TableMeta meta; \
        meta.tableName = QStringLiteral(tableName); \
        meta.primaryKey = "id"; \
        return meta; \
    }

} // namespace orm
} // namespace sc

#endif