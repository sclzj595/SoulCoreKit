#ifndef SOUL_AUTH_PERMISSION_H
#define SOUL_AUTH_PERMISSION_H

#include <QString>
#include <QSet>
#include <QMap>
#include <vector>

namespace sc {

/**
 * @class Permission
 * @brief 权限定义和检查类
 *
 * Permission 提供权限的定义和检查功能，支持：
 * - 权限注册和管理
 * - 权限检查
 * - 角色与权限的映射
 *
 * 使用方式：
 * @code
 * Permission perm;
 * perm.registerPermission("create_user", "创建用户");
 * perm.registerPermission("delete_user", "删除用户");
 *
 * perm.registerRole("admin", {"create_user", "delete_user"});
 * perm.registerRole("user", {"create_user"});
 *
 * if (perm.hasPermission("admin", "create_user")) {
 *     // 有权限
 * }
 * @endcode
 *
 * @see AuthManager
 */
class Permission {
public:
    /**
     * @brief 权限信息结构体
     */
    struct PermissionInfo {
        QString id;
        QString name;
        QString description;
    };

    /**
     * @brief 构造函数
     */
    Permission();

    /**
     * @brief 析构函数
     */
    ~Permission() = default;

    /**
     * @brief 注册权限
     * @param id 权限 ID
     * @param name 权限名称
     * @param description 权限描述（可选）
     */
    void registerPermission(const QString& id, const QString& name, const QString& description = "");

    /**
     * @brief 注册角色
     * @param role 角色名称
     * @param permissions 角色拥有的权限列表
     */
    void registerRole(const QString& role, const std::vector<QString>& permissions);

    /**
     * @brief 检查角色是否具有指定权限
     * @param role 角色名称
     * @param permissionId 权限 ID
     * @return 如果有权限返回 true
     */
    bool hasPermission(const QString& role, const QString& permissionId) const;

    /**
     * @brief 检查角色是否具有任意一个指定权限
     * @param role 角色名称
     * @param permissionIds 权限 ID 列表
     * @return 如果具有任意一个权限返回 true
     */
    bool hasAnyPermission(const QString& role, const std::vector<QString>& permissionIds) const;

    /**
     * @brief 检查角色是否具有所有指定权限
     * @param role 角色名称
     * @param permissionIds 权限 ID 列表
     * @return 如果具有所有权限返回 true
     */
    bool hasAllPermissions(const QString& role, const std::vector<QString>& permissionIds) const;

    /**
     * @brief 获取角色的所有权限
     * @param role 角色名称
     * @return 权限列表
     */
    std::vector<QString> getRolePermissions(const QString& role) const;

    /**
     * @brief 获取所有已注册的权限
     * @return 权限信息列表
     */
    std::vector<PermissionInfo> getAllPermissions() const;

    /**
     * @brief 获取所有已注册的角色
     * @return 角色名称列表
     */
    std::vector<QString> getAllRoles() const;

    /**
     * @brief 检查权限是否已注册
     * @param permissionId 权限 ID
     * @return 如果已注册返回 true
     */
    bool isPermissionRegistered(const QString& permissionId) const;

    /**
     * @brief 检查角色是否已注册
     * @param role 角色名称
     * @return 如果已注册返回 true
     */
    bool isRoleRegistered(const QString& role) const;

private:
    QMap<QString, PermissionInfo> m_permissions;
    QMap<QString, QSet<QString>> m_rolePermissions;
};

}

#endif
