#ifndef SOUL_AUTH_PERMISSIONS_H
#define SOUL_AUTH_PERMISSIONS_H

#include <QString>
#include <QStringList>
#include <functional>

namespace sc {

/**
 * @namespace Permissions
 * @brief 权限验证命名空间
 *
 * Permissions 提供权限验证的工具函数和常量定义。
 * 支持：
 * - 权限常量定义
 * - 权限验证
 * - 角色权限映射
 *
 * 使用方式：
 * @code
 * // 检查权限
 * bool hasPermission = Permissions::check(user, Permissions::READ_USER);
 *
 * // 检查多个权限（任一）
 * bool hasAny = Permissions::checkAny(user, {Permissions::READ_USER, Permissions::WRITE_USER});
 *
 * // 检查多个权限（全部）
 * bool hasAll = Permissions::checkAll(user, {Permissions::READ_USER, Permissions::WRITE_USER});
 * @endcode
 *
 * @see AuthManager, User
 */
namespace Permissions {

/**
 * @brief 权限常量定义
 */
const QString READ_USER = "user:read";
const QString WRITE_USER = "user:write";
const QString DELETE_USER = "user:delete";
const QString READ_ROLE = "role:read";
const QString WRITE_ROLE = "role:write";
const QString READ_CONFIG = "config:read";
const QString WRITE_CONFIG = "config:write";
const QString READ_LOG = "log:read";
const QString READ_STATISTICS = "statistics:read";
const QString ADMIN = "admin";

/**
 * @brief 角色到权限的映射
 * @param role 角色名称
 * @return 该角色拥有的权限列表
 */
QStringList rolePermissions(const QString& role);

/**
 * @brief 检查用户是否拥有指定权限
 * @tparam UserType 用户类型
 * @param user 用户对象
 * @param permission 权限标识
 * @return 如果拥有返回 true
 */
template<typename UserType>
bool check(const UserType& user, const QString& permission) {
    if (user.hasRole("admin")) {
        return true;
    }
    for (const QString& role : user.roles()) {
        if (rolePermissions(role).contains(permission)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查用户是否拥有任一指定权限
 * @tparam UserType 用户类型
 * @param user 用户对象
 * @param permissions 权限列表
 * @return 如果拥有任一权限返回 true
 */
template<typename UserType>
bool checkAny(const UserType& user, const QStringList& permissions) {
    for (const QString& permission : permissions) {
        if (check(user, permission)) {
            return true;
        }
    }
    return false;
}

/**
 * @brief 检查用户是否拥有所有指定权限
 * @tparam UserType 用户类型
 * @param user 用户对象
 * @param permissions 权限列表
 * @return 如果拥有所有权限返回 true
 */
template<typename UserType>
bool checkAll(const UserType& user, const QStringList& permissions) {
    for (const QString& permission : permissions) {
        if (!check(user, permission)) {
            return false;