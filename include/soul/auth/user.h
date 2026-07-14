#ifndef SOUL_AUTH_USER_H
#define SOUL_AUTH_USER_H

#include <QString>
#include <QVariantMap>

namespace sc {

/**
 * @class User
 * @brief 用户信息类
 *
 * User 封装了用户的基本信息，包括：
 * - 用户ID
 * - 用户名
 * - 邮箱
 * - 角色列表
 * - 自定义属性
 *
 * 使用方式：
 * @code
 * User user("1", "admin", "admin@example.com");
 * user.addRole("admin");
 * user.setProperty("displayName", "管理员");
 * @endcode
 *
 * @see AuthManager, Token
 */
class User {
public:
    /**
     * @brief 默认构造函数
     */
    User() = default;

    /**
     * @brief 构造函数
     * @param id 用户ID
     * @param username 用户名
     * @param email 邮箱
     */
    User(const QString& id, const QString& username, const QString& email = "");

    /**
     * @brief 获取用户ID
     * @return 用户ID
     */
    QString id() const;

    /**
     * @brief 设置用户ID
     * @param id 用户ID
     */
    void setId(const QString& id);

    /**
     * @brief 获取用户名
     * @return 用户名
     */
    QString username() const;

    /**
     * @brief 设置用户名
     * @param username 用户名
     */
    void setUsername(const QString& username);

    /**
     * @brief 获取邮箱
     * @return 邮箱
     */
    QString email() const;

    /**
     * @brief 设置邮箱
     * @param email 邮箱
     */
    void setEmail(const QString& email);

    /**
     * @brief 获取角色列表
     * @return 角色列表
     */
    QStringList roles() const;

    /**
     * @brief 添加角色
     * @param role 角色名称
     */
    void addRole(const QString& role);

    /**
     * @brief 移除角色
     * @param role 角色名称
     */
    void removeRole(const QString& role);

    /**
     * @brief 检查是否拥有指定角色
     * @param role 角色名称
     * @return 如果拥有返回 true
     */
    bool hasRole(const QString& role) const;

    /**
     * @brief 获取自定义属性
     * @param key 属性键
     * @return 属性值，如果不存在返回空字符串
     */
    QString getProperty(const QString& key) const;

    /**
     * @brief 设置自定义属性
     * @param key 属性键
     * @param value 属性值
     */
    void setProperty(const QString& key, const QString& value);

    /**
     * @brief 检查用户是否已登录
     * @return 如果已登录返回 true
     */
    bool isLoggedIn() const;

    /**
     * @brief 转换为 QVariantMap
     * @return QVariantMap
     */
    QVariantMap toMap() const;

    /**
     * @brief 从 QVariantMap 创建 User
     * @param map QVariantMap
     * @return User 对象
     */
    static User fromMap(const QVariantMap& map);

private:
    QString m_id;
    QString m_username;
    QString m_email;
    QStringList m_roles;
    QVariantMap m_properties;
};

}

#endif
