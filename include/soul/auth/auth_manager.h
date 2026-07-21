#ifndef SOUL_AUTH_AUTH_MANAGER_H
#define SOUL_AUTH_AUTH_MANAGER_H

#include <QString>
#include <QJsonObject>
#include <functional>
#include <memory>
#include <QMutex>
#include <QMutexLocker>
#include "soul/core/result.h"
#include "soul/core/error.h"
#include "soul/base/base_manager.h"
#include "soul/core/singleton.h"

namespace sc {

/**
 * @class AuthManager
 * @brief 认证管理器，提供用户登录、登出和权限验证功能
 *
 * AuthManager 采用单例模式，管理用户的认证状态，包括：
 * - 用户登录/登出
 * - Token 获取和刷新
 * - 权限验证
 * - 认证状态监听
 *
 * 使用方式：
 * @code
 * // 初始化
 * AuthManager::instance().init();
 *
 * // 登录
 * auto result = AuthManager::instance().login("username", "password");
 * if (result.isOk()) {
 *     // 登录成功
 * }
 *
 * // 检查是否已认证
 * if (AuthManager::instance().isAuthenticated()) {
 *     // 已登录
 * }
 *
 * // 权限验证
 * if (AuthManager::instance().hasPermission("admin")) {
 *     // 有权限
 * }
 *
 * // 登出
 * AuthManager::instance().logout();
 *
 * // 清理
 * AuthManager::instance().shutdown();
 * @endcode
 *
 * @see TokenManager, Permission
 */
class AuthManager : public BaseManager, public Singleton<AuthManager> {
    Q_OBJECT
    friend class Singleton<AuthManager>;
public:
    /**
     * @brief 用户信息结构体
     */
    struct UserInfo {
        QString userId;
        QString username;
        QString email;
        QString role;
        QString token;
        QString refreshToken;
        int expiresIn;
        QJsonObject extra;
    };

    /**
     * @brief 认证状态改变回调类型
     */
    using AuthStateCallback = std::function<void(bool authenticated)>;

    /**
     * @brief 用户登录验证回调类型
     */
    using LoginValidator = std::function<Result<UserInfo>(const QString& username, const QString& password)>;

    /**
     * @brief 设置登录验证回调
     * @param validator 验证回调函数
     */
    void setLoginValidator(LoginValidator validator);

    /**
     * @brief 用户登录
     * @param username 用户名
     * @param password 密码
     * @return 包含用户信息的 Result
     */
    Result<UserInfo> login(const QString& username, const QString& password);

    /**
     * @brief 用户登出
     */
    void logout();

    /**
     * @brief 使用 Refresh Token 刷新访问 Token
     * @return 包含新 Token 的 Result
     */
    Result<QString> refreshToken();

    /**
     * @brief 检查是否已认证
     * @return 如果已认证返回 true
     */
    bool isAuthenticated() const;

    /**
     * @brief 获取当前用户信息
     * @return 用户信息，如果未登录返回空对象
     */
    UserInfo currentUser() const;

    /**
     * @brief 获取当前访问 Token
     * @return Token 字符串，如果未登录返回空字符串
     */
    QString accessToken() const;

    /**
     * @brief 获取当前 Refresh Token
     * @return Refresh Token 字符串
     */
    QString refreshToken() const;

    /**
     * @brief 检查是否具有指定权限
     * @param permission 权限名称
     * @return 如果有权限返回 true
     */
    bool hasPermission(const QString& permission) const;

    /**
     * @brief 检查是否具有指定角色
     * @param role 角色名称
     * @return 如果有角色返回 true
     */
    bool hasRole(const QString& role) const;

    /**
     * @brief 设置认证状态改变回调
     * @param callback 回调函数
     */
    void setAuthStateCallback(AuthStateCallback callback);

    /**
     * @brief 从存储中加载认证状态
     * @return 如果成功返回 true
     */
    bool loadAuthState();

    /**
     * @brief 保存认证状态到存储
     * @return 如果成功返回 true
     */
    bool saveAuthState();

    /**
     * @brief 初始化管理器
     * @return 初始化成功返回 true，失败返回 false
     */
    bool init() override;

    /**
     * @brief 清理管理器
     */
    void shutdown() override;

private:
    AuthManager();
    ~AuthManager() = default;

    mutable QMutex m_mutex;
    UserInfo m_userInfo;
    bool m_authenticated = false;
    AuthStateCallback m_authStateCallback;
    LoginValidator m_loginValidator;

    void notifyAuthStateChanged(bool authenticated);
};

}

#endif
