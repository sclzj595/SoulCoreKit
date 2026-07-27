#include "soul/auth/auth_manager.h"
#include "soul/network/session.h"
#include "soul/storage/memory_storage.h"
#include "soul/utils/json/json_utils.h"
#include "soul/auth/permission.h"

namespace sc {

AuthManager::AuthManager() : BaseManager() {
    m_storage.open("auth");
}

Result<void> AuthManager::init() {
    QMutexLocker lock(&m_mutex);
    if (m_initialized) {
        return Ok();
    }
    auto loadResult = loadAuthState();
    if (!loadResult.isOk()) {
        return loadResult.unwrapErr();
    }
    m_initialized = true;
    return Ok();
}

void AuthManager::shutdown() {
    QMutexLocker lock(&m_mutex);
    logout();
    m_initialized = false;
}

void AuthManager::setLoginValidator(LoginValidator validator) {
    QMutexLocker lock(&m_mutex);
    m_loginValidator = validator;
}

Result<AuthManager::UserInfo> AuthManager::login(const QString& username, const QString& password) {
    if (username.isEmpty() || password.isEmpty()) {
        return Error(ErrorCode::AuthError, "Username and password are required");
    }

    LoginValidator validator;
    {
        QMutexLocker lock(&m_mutex);
        validator = m_loginValidator;
    }

    UserInfo info;
    if (validator) {
        auto result = validator(username, password);
        if (!result.isOk()) {
            return result.unwrapErr();
        }
        info = result.unwrap();
    } else {
        if (username != "testuser" || password != "password") {
            return Error(ErrorCode::AuthError, "Invalid username or password");
        }

        info.userId = "1";
        info.username = username;
        info.email = "test@example.com";
        info.role = "admin";
        info.token = "Bearer test_token";
        info.refreshToken = "refresh_token";
        info.expiresIn = 3600;
    }

    {
        QMutexLocker lock(&m_mutex);
        m_userInfo = info;
        m_authenticated = true;
    }

    sc::network::Session::instance().setToken(info.token);
    sc::network::Session::instance().set("username", info.username);
    sc::network::Session::instance().set("role", info.role);

    saveAuthState();
    notifyAuthStateChanged(true);

    return Result<UserInfo>(info);
}

void AuthManager::logout() {
    {
        QMutexLocker lock(&m_mutex);
        m_authenticated = false;
        m_userInfo = UserInfo();
    }

    saveAuthState();  // 清除存储中的 auth_token 等凭据,防止重启后恢复登录态
    sc::network::Session::instance().clear();
    notifyAuthStateChanged(false);
}

Result<QString> AuthManager::refreshToken() {
    QString refreshToken;
    {
        QMutexLocker lock(&m_mutex);
        refreshToken = m_userInfo.refreshToken;
    }

    if (refreshToken.isEmpty()) {
        return Error(ErrorCode::AuthError, "No refresh token available");
    }

    QString newToken = "Bearer refreshed_token_" + QString::number(QDateTime::currentMSecsSinceEpoch());

    {
        QMutexLocker lock(&m_mutex);
        m_userInfo.token = newToken;
    }

    sc::network::Session::instance().setToken(newToken);
    saveAuthState();
    return Result<QString>(newToken);
}

bool AuthManager::isAuthenticated() const {
    QMutexLocker lock(&m_mutex);
    return m_authenticated;
}

AuthManager::UserInfo AuthManager::currentUser() const {
    QMutexLocker lock(&m_mutex);
    return m_userInfo;
}

QString AuthManager::accessToken() const {
    QMutexLocker lock(&m_mutex);
    return m_userInfo.token;
}

QString AuthManager::refreshToken() const {
    QMutexLocker lock(&m_mutex);
    return m_userInfo.refreshToken;
}

bool AuthManager::hasPermission(const QString& permission) const {
    QString role;
    bool authenticated;
    {
        QMutexLocker lock(&m_mutex);
        role = m_userInfo.role;
        authenticated = m_authenticated;
    }

    if (!authenticated || role.isEmpty()) {
        return false;
    }

    if (role == "admin") {
        return true;
    }

    Permission perm;
    if (perm.isRoleRegistered(role)) {
        return perm.hasPermission(role, permission);
    }

    return false;
}

bool AuthManager::hasRole(const QString& role) const {
    QMutexLocker lock(&m_mutex);
    return m_userInfo.role == role;
}

void AuthManager::setAuthStateCallback(AuthStateCallback callback) {
    QMutexLocker lock(&m_mutex);
    m_authStateCallback = callback;
}

Result<void> AuthManager::loadAuthState() {
    auto result = m_storage.get("auth_token");
    QString token = result.unwrapOr("");
    if (!token.isEmpty()) {
        QString username = m_storage.get("auth_username").unwrapOr("");
        QString role = m_storage.get("auth_role").unwrapOr("");
        QString refreshToken = m_storage.get("auth_refresh_token").unwrapOr("");

        {
            QMutexLocker lock(&m_mutex);
            m_authenticated = true;
            m_userInfo.token = token;
            m_userInfo.username = username;
            m_userInfo.role = role;
            m_userInfo.refreshToken = refreshToken;
        }

        sc::network::Session::instance().setToken(token);
    }

    return {};
}

Result<void> AuthManager::saveAuthState() {
    QString token, username, role, refreshToken;
    bool authenticated;
    {
        QMutexLocker lock(&m_mutex);
        token = m_userInfo.token;
        username = m_userInfo.username;
        role = m_userInfo.role;
        refreshToken = m_userInfo.refreshToken;
        authenticated = m_authenticated;
    }

    if (authenticated) {
        m_storage.put("auth_token", token);
        m_storage.put("auth_username", username);
        m_storage.put("auth_role", role);
        m_storage.put("auth_refresh_token", refreshToken);
    } else {
        m_storage.remove("auth_token");
        m_storage.remove("auth_username");
        m_storage.remove("auth_role");
        m_storage.remove("auth_refresh_token");
    }

    return {};
}

void AuthManager::notifyAuthStateChanged(bool authenticated) {
    AuthStateCallback callback;
    {
        QMutexLocker lock(&m_mutex);
        callback = m_authStateCallback;
    }
    if (callback) {
        callback(authenticated);
    }
}

}
