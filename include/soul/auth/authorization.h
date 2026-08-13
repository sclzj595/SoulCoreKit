#ifndef SOUL_AUTH_AUTHORIZATION_H
#define SOUL_AUTH_AUTHORIZATION_H

// ============================================================================
// authorization.h — 授权层 (What can you do?) [v2.7.0 新增]
// ============================================================================
//
// 职责: 检查已认证身份是否拥有执行某个操作的权限
// 输入: Identity + Permission/Resource
// 输出: bool (是否允许)
//
// 与 Authentication 的关系:
//   Authentication → Identity → Authorization → Policy
//
// 用法:
//   auto authz = std::make_shared<RoleBasedAuthorizer>();
//   if (authz->isAuthorized(identity, "users:write")) {
//       // 允许操作
//   }

#include <QString>
#include <QVector>
#include <memory>
#include <string>
#include <functional>

#include "soul/core/result.h"
#include "soul/auth/authentication.h"

namespace sc {
namespace auth {

// ============================================================================
// Permission — 权限 [v2.7.0]
// ============================================================================

struct Permission {
    QString resource;   // 资源: "users", "files", "settings"
    QString action;     // 操作: "read", "write", "delete", "admin"
    QString scope;      // 作用域: "own", "group", "all" (默认 "all")

    /// @brief 格式化权限字符串: "resource:action" 或 "resource:action:scope"
    QString toString() const {
        if (scope.isEmpty() || scope == "all") {
            return resource + ":" + action;
        }
        return resource + ":" + action + ":" + scope;
    }

    /// @brief 从字符串解析: "users:write", "files:read:own"
    static Permission fromString(const QString& str) {
        auto parts = str.split(':');
        if (parts.size() >= 2) {
            return {
                parts[0],
                parts[1],
                parts.size() >= 3 ? parts[2] : QString("all")
            };
        }
        return {};
    }
};

// ============================================================================
// IAuthorizer — 授权器接口 [v2.7.0]
// ============================================================================

class IAuthorizer {
public:
    virtual ~IAuthorizer() = default;

    /// @brief 检查身份是否有权限执行操作
    virtual bool isAuthorized(const Identity& identity, const Permission& permission) = 0;

    /// @brief 检查身份是否拥有指定角色
    virtual bool hasRole(const Identity& identity, const QString& role) = 0;

    /// @return 授权器名称
    virtual std::string name() const = 0;
};

// ============================================================================
// RoleBasedAuthorizer — 基于角色的授权器 [v2.7.0]
// ============================================================================
// 角色 → 权限映射。
// 例如: admin → {users:*, files:*, settings:*}

class RoleBasedAuthorizer : public IAuthorizer {
public:
    /// @brief 设置角色权限映射
    void setRolePermissions(const QString& role, const QVector<Permission>& permissions);

    /// @brief 为角色添加权限
    void addPermission(const QString& role, const Permission& permission);

    /// @brief 为用户添加直接权限 (不通过角色)
    void addUserPermission(const QString& userId, const Permission& permission);

    bool isAuthorized(const Identity& identity, const Permission& permission) override;
    bool hasRole(const Identity& identity, const QString& role) override;
    std::string name() const override { return "RoleBasedAuthorizer"; }

private:
    QMap<QString, QVector<Permission>> m_rolePermissions;
    QMap<QString, QVector<Permission>> m_userPermissions;
    mutable std::mutex m_mutex;
};

// ============================================================================
// PolicyBasedAuthorizer — 基于策略的授权器 [v2.7.0]
// ============================================================================
// 可编程策略评估。适用于复杂权限场景 (ABAC)。
// 策略函数: (Identity, Permission) → bool

class PolicyBasedAuthorizer : public IAuthorizer {
public:
    using PolicyFn = std::function<bool(const Identity&, const Permission&)>;

    /// @brief 添加策略
    void addPolicy(PolicyFn policy);

    bool isAuthorized(const Identity& identity, const Permission& permission) override;
    bool hasRole(const Identity& identity, const QString& role) override;
    std::string name() const override { return "PolicyBasedAuthorizer"; }

private:
    std::vector<PolicyFn> m_policies;
    mutable std::mutex m_mutex;
};

} // namespace auth
} // namespace sc

#endif // SOUL_AUTH_AUTHORIZATION_H
