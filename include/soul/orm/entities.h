#ifndef SOUL_ORM_ENTITIES_H
#define SOUL_ORM_ENTITIES_H

#include "soul/orm/entity.h"

namespace sc {
namespace orm {

class User : public Entity<User> {
public:
    SC_TABLE(User, "user")

    SC_FIELD(QString, username)
    SC_FIELD(QString, email)
    SC_FIELD(QString, password)
    SC_FIELD(QString, role)

    static TableMeta tableMeta() {
        TableMeta meta;
        meta.tableName = QStringLiteral("user");
        meta.primaryKey = "id";
        meta.fields["id"] = {"id", "id", "QString", true, false, false, ""};
        meta.fields["createTime"] = {"createTime", "create_time", "QDateTime", false, false, true, ""};
        meta.fields["updateTime"] = {"updateTime", "update_time", "QDateTime", false, false, true, ""};
        meta.fields["deleted"] = {"deleted", "deleted", "int", false, false, true, "0"};
        meta.fields["username"] = {"username", "username", "QString", false, false, true, ""};
        meta.fields["email"] = {"email", "email", "QString", false, false, true, ""};
        meta.fields["password"] = {"password", "password", "QString", false, false, true, ""};
        meta.fields["role"] = {"role", "role", "QString", false, false, true, ""};
        return meta;
    }

    QVariant getPropertyImpl(const QString& prop) const {
        if (prop == "id") return id;
        if (prop == "createTime") return createTime;
        if (prop == "updateTime") return updateTime;
        if (prop == "deleted") return deleted;
        if (prop == "username") return QVariant::fromValue(username);
        if (prop == "email") return QVariant::fromValue(email);
        if (prop == "password") return QVariant::fromValue(password);
        if (prop == "role") return QVariant::fromValue(role);
        return QVariant();
    }

    void setPropertyImpl(const QString& prop, const QVariant& value) {
        if (prop == "id") { id = value.toString(); return; }
        if (prop == "createTime") { createTime = value.toDateTime(); return; }
        if (prop == "updateTime") { updateTime = value.toDateTime(); return; }
        if (prop == "deleted") { deleted = value.toInt(); return; }
        if (prop == "username") { username = value.value<QString>(); return; }
        if (prop == "email") { email = value.value<QString>(); return; }
        if (prop == "password") { password = value.value<QString>(); return; }
        if (prop == "role") { role = value.value<QString>(); return; }
    }
};

} // namespace orm
} // namespace sc

#endif
