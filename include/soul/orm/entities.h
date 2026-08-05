#ifndef SOUL_ORM_ENTITIES_H
#define SOUL_ORM_ENTITIES_H

#include "soul/orm/entity.h"
#include "soul/orm/column.h"

namespace sc {
namespace orm {

class User : public Entity<User> {
public:
    SC_ENTITY_TABLE(User, "user")

    SC_FIELD(QString, username)
    SC_FIELD(QString, email)
    SC_FIELD(QString, password)
    SC_FIELD(QString, role)

    // 类型安全列引用（C++17 static inline const，零运行时开销）
    // 与上方 SC_FIELD 字段类型严格对应，用于 TypedQueryWrapper
    static inline const Column<User, QString>    IdCol       {"id"};
    static inline const Column<User, QDateTime>  CreateTimeCol{"createTime"};
    static inline const Column<User, QDateTime>  UpdateTimeCol{"updateTime"};
    static inline const Column<User, int>        DeletedCol  {"deleted"};
    static inline const Column<User, QString>    UsernameCol {"username"};
    static inline const Column<User, QString>    EmailCol    {"email"};
    static inline const Column<User, QString>    PasswordCol {"password"};
    static inline const Column<User, QString>    RoleCol     {"role"};

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
