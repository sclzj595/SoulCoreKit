#include "soul/auth/permission.h"

namespace sc {

Permission::Permission() {
}

void Permission::registerPermission(const QString& id, const QString& name, const QString& description) {
    PermissionInfo info;
    info.id = id;
    info.name = name;
    info.description = description;
    m_permissions[id] = info;
}

void Permission::registerRole(const QString& role, const std::vector<QString>& permissions) {
    QSet<QString> permSet;
    for (const QString& perm : permissions) {
        if (m_permissions.contains(perm)) {
            permSet.insert(perm);
        }
    }
    m_rolePermissions[role] = permSet;
}

bool Permission::hasPermission(const QString& role, const QString& permissionId) const {
    if (!m_rolePermissions.contains(role)) {
        return false;
    }

    return m_rolePermissions[role].contains(permissionId);
}

bool Permission::hasAnyPermission(const QString& role, const std::vector<QString>& permissionIds) const {
    if (!m_rolePermissions.contains(role)) {
        return false;
    }

    const QSet<QString>& perms = m_rolePermissions[role];
    for (const QString& perm : permissionIds) {
        if (perms.contains(perm)) {
            return true;
        }
    }

    return false;
}

bool Permission::hasAllPermissions(const QString& role, const std::vector<QString>& permissionIds) const {
    if (!m_rolePermissions.contains(role)) {
        return false;
    }

    const QSet<QString>& perms = m_rolePermissions[role];
    for (const QString& perm : permissionIds) {
        if (!perms.contains(perm)) {
            return false;
        }
    }

    return true;
}

std::vector<QString> Permission::getRolePermissions(const QString& role) const {
    std::vector<QString> result;

    if (m_rolePermissions.contains(role)) {
        const QSet<QString>& perms = m_rolePermissions[role];
        for (const QString& perm : perms) {
            result.push_back(perm);
        }
    }

    return result;
}

std::vector<Permission::PermissionInfo> Permission::getAllPermissions() const {
    std::vector<PermissionInfo> result;

    for (auto it = m_permissions.begin(); it != m_permissions.end(); ++it) {
        result.push_back(it.value());
    }

    return result;
}

std::vector<QString> Permission::getAllRoles() const {
    std::vector<QString> result;

    for (auto it = m_rolePermissions.begin(); it != m_rolePermissions.end(); ++it) {
        result.push_back(it.key());
    }

    return result;
}

bool Permission::isPermissionRegistered(const QString& permissionId) const {
    return m_permissions.contains(permissionId);
}

bool Permission::isRoleRegistered(const QString& role) const {
    return m_rolePermissions.contains(role);
}

}
