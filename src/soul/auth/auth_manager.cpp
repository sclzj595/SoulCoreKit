#include "soul/auth/auth_manager.h"
#include "soul/network/session.h"
#include "soul/storage/memory_storage.h"
#include "soul/utils/json/json_utils.h"
#include "soul/auth/permission.h"

namespace sc {

AuthManager::AuthManager() : BaseManager() {
}

bool AuthManager::init() {
    QMutexLocker lock(&m_mutex);
    if (m_initialized) {
        return true;
    }
    loadAuthState();
    m_initialized = true;
    return true;
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

    sc::network::Session::instance().clear();
    saveAuthState();
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

bool AuthManager::loadAuthState() {
    MemoryStorage storage;
    auto result = storage.open("auth");
    if (!result.isOk()) {
        return false;
    }

    auto tokenResult = storage.get("auth_token");
    QString token = tokenResult.unwrapOr("");
    if (!token.isEmpty()) {
        QString username = storage.get("auth_username").unwrapOr("");
        QString role = storage.get("auth_role").unwrapOr("");
        QString refreshToken = storage.get("auth_refresh_token").unwrapOr("");

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

    return true;
}

bool AuthManager::saveAuthState() {
    MemoryStorage storage;
    auto result = storage.open("auth");
    if (!result.isOk()) {
        return false;
    }

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
        storage.put("auth_token", token);
        storage.put("auth_username", username);
        storage.put("auth_role", role);
        storage.put("auth_refresh_token", refreshToken);
    } else {
        storage.remove("auth_token");
        storage.remove("auth_username");
        storage.remove("auth_role");
        storage.remove("auth_refresh_token");
    }

    return true;
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
