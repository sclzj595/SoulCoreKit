#ifndef SOUL_ORM_ENTITIES_H
#define SOUL_ORM_ENTITIES_H

#include "soul/orm/entity.h"

namespace sc {
namespace orm {

class User : public BaseEntity {
public:
    TABLE(user);

    QString username;
    QString email;
    QString password;
    QString role;

    TableMeta getTableMeta() const override {
        TableMeta meta;
        meta.tableName = "user";
        meta.primaryKey = "id";

        FieldMeta usernameField;
        usernameField.name = "username";
        usernameField.columnName = "username";
        usernameField.type = "VARCHAR(255)";
        usernameField.isNullable = false;
        meta.fields["username"] = usernameField;

        FieldMeta emailField;
        emailField.name = "email";
        emailField.columnName = "email";
        emailField.type = "VARCHAR(255)";
        emailField.isNullable = false;
        meta.fields["email"] = emailField;

        FieldMeta passwordField;
        passwordField.name = "password";
        passwordField.columnName = "password";
        passwordField.type = "VARCHAR(255)";
        passwordField.isNullable = false;
        meta.fields["password"] = passwordField;

        FieldMeta roleField;
        roleField.name = "role";
        roleField.columnName = "role";
        roleField.type = "VARCHAR(50)";
        roleField.isNullable = true;
        meta.fields["role"] = roleField;

        return meta;
    }

    QVariant getProperty(const QString& name) const override {
        if (name == "username") return username;
        if (name == "email") return email;
        if (name == "password") return password;
        if (name == "role") return role;
        if (name == "id") return id;
        if (name == "createTime") return createTime;
        if (name == "updateTime") return updateTime;
        if (name == "deleted") return deleted;
        return QVariant();
    }

    void setProperty(const QString& name, const QVariant& value) override {
        if (name == "username") username = value.toString();
        else if (name == "email") email = value.toString();
        else if (name == "password") password = value.toString();
        else if (name == "role") role = value.toString();
        else if (name == "id") id = value.toString();
        else if (name == "createTime") createTime = value.toDateTime();
        else if (name == "updateTime") updateTime = value.toDateTime();
        else if (name == "deleted") deleted = value.toInt();
    }
};

class Config : public BaseEntity {
public:
    TABLE(config);

    QString key;
    QString value;
    QString category;

    TableMeta getTableMeta() const override {
        TableMeta meta;
        meta.tableName = "config";
        meta.primaryKey = "id";

        FieldMeta keyField;
        keyField.name = "key";
        keyField.columnName = "config_key";
        keyField.type = "VARCHAR(255)";
        keyField.isNullable = false;
        meta.fields["key"] = keyField;

        FieldMeta valueField;
        valueField.name = "value";
        valueField.columnName = "config_value";
        valueField.type = "TEXT";
        valueField.isNullable = true;
        meta.fields["value"] = valueField;

        FieldMeta categoryField;
        categoryField.name = "category";
        categoryField.columnName = "category";
        categoryField.type = "VARCHAR(100)";
        categoryField.isNullable = true;
        meta.fields["category"] = categoryField;

        return meta;
    }

    QVariant getProperty(const QString& name) const override {
        if (name == "key") return key;
        if (name == "value") return value;
        if (name == "category") return category;
        if (name == "id") return id;
        if (name == "createTime") return createTime;
        if (name == "updateTime") return updateTime;
        if (name == "deleted") return deleted;
        return QVariant();
    }

    void setProperty(const QString& name, const QVariant& val) override {
        if (name == "key") key = val.toString();
        else if (name == "value") this->value = val.toString();
        else if (name == "category") category = val.toString();
        else if (name == "id") id = val.toString();
        else if (name == "createTime") createTime = val.toDateTime();
        else if (name == "updateTime") updateTime = val.toDateTime();
        else if (name == "deleted") deleted = val.toInt();
    }
};

class AuditLog : public BaseEntity {
public:
    TABLE(audit_log);

    QString action;
    QString resource;
    QString operatorId;
    QString detail;

    TableMeta getTableMeta() const override {
        TableMeta meta;
        meta.tableName = "audit_log";
        meta.primaryKey = "id";

        FieldMeta actionField;
        actionField.name = "action";
        actionField.columnName = "action";
        actionField.type = "VARCHAR(100)";
        actionField.isNullable = false;
        meta.fields["action"] = actionField;

        FieldMeta resourceField;
        resourceField.name = "resource";
        resourceField.columnName = "resource";
        resourceField.type = "VARCHAR(255)";
        resourceField.isNullable = true;
        meta.fields["resource"] = resourceField;

        FieldMeta operatorField;
        operatorField.name = "operatorId";
        operatorField.columnName = "operator_id";
        operatorField.type = "VARCHAR(64)";
        operatorField.isNullable = true;
        meta.fields["operatorId"] = operatorField;

        FieldMeta detailField;
        detailField.name = "detail";
        detailField.columnName = "detail";
        detailField.type = "TEXT";
        detailField.isNullable = true;
        meta.fields["detail"] = detailField;

        return meta;
    }

    QVariant getProperty(const QString& name) const override {
        if (name == "action") return action;
        if (name == "resource") return resource;
        if (name == "operatorId") return operatorId;
        if (name == "detail") return detail;
        if (name == "id") return id;
        if (name == "createTime") return createTime;
        if (name == "updateTime") return updateTime;
        if (name == "deleted") return deleted;
        return QVariant();
    }

    void setProperty(const QString& name, const QVariant& value) override {
        if (name == "action") action = value.toString();
        else if (name == "resource") resource = value.toString();
        else if (name == "operatorId") operatorId = value.toString();
        else if (name == "detail") detail = value.toString();
        else if (name == "id") id = value.toString();
        else if (name == "createTime") createTime = value.toDateTime();
        else if (name == "updateTime") updateTime = value.toDateTime();
        else if (name == "deleted") deleted = value.toInt();
    }
};

} // namespace orm
} // namespace sc

#endif