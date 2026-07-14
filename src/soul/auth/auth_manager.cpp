#include "soul/auth/auth_manager.h"
#include "soul/network/session.h"
#include "soul/storage/memory_storage.h"
#include "soul/utils/json/json_utils.h"

namespace sc {

AuthManager::AuthManager() : BaseManager() {
}

bool AuthManager::init() {
    if (m_initialized) {
        return true;
    }
    loadAuthState();
    m_initialized = true;
    return true;
}

void AuthManager::shutdown() {
    logout();
    m_initialized = false;
}

Result<AuthManager::UserInfo> AuthManager::login(const QString& username, const QString& password) {
    Q_UNUSED(username);
    Q_UNUSED(password);

    UserInfo info;
    info.userId = "1";
    info.username = username;
    info.email = "test@example.com";
    info.role = "admin";
    info.token = "Bearer test_token";
    info.refreshToken = "refresh_token";
    info.expiresIn = 3600;

    m_userInfo = info;
    m_authenticated = true;

    Session::instance().setToken(info.token);
    Session::instance().set("username", info.username);
    Session::instance().set("role", info.role);

    saveAuthState();
    notifyAuthStateChanged(true);

    return Result<UserInfo>(info);
}

void AuthManager::logout() {
    m_authenticated = false;
    m_userInfo = UserInfo();

    Session::instance().clear();
    saveAuthState();
    notifyAuthStateChanged(false);
}

Result<QString> AuthManager::refreshToken() {
    QString newToken = "Bearer refreshed_token";
    m_userInfo.token = newToken;
    Session::instance().setToken(newToken);
    saveAuthState();
    return Result<QString>(newToken);
}

bool AuthManager::isAuthenticated() const {
    return m_authenticated;
}

AuthManager::UserInfo AuthManager::currentUser() const {
    return m_userInfo;
}

QString AuthManager::accessToken() const {
    return m_userInfo.token;
}

QString AuthManager::refreshToken() const {
    return m_userInfo.refreshToken;
}

bool AuthManager::hasPermission(const QString& permission) const {
    Q_UNUSED(permission);
    return m_userInfo.role == "admin";
}

bool AuthManager::hasRole(const QString& role) const {
    return m_userInfo.role == role;
}

void AuthManager::setAuthStateCallback(AuthStateCallback callback) {
    m_authStateCallback = callback;
}

bool AuthManager::loadAuthState() {
    MemoryStorage storage;
    auto result = storage.open("auth");
    if (!result.isOk()) {
        return false;
    }

    QString token = storage.get("auth_token");
    if (!token.isEmpty()) {
        m_authenticated = true;
        m_userInfo.token = token;
        m_userInfo.username = storage.get("auth_username");
        m_userInfo.role = storage.get("auth_role");
        m_userInfo.refreshToken = storage.get("auth_refresh_token");
        Session::instance().setToken(token);
    }

    return true;
}

bool AuthManager::saveAuthState() {
    MemoryStorage storage;
    auto result = storage.open("auth");
    if (!result.isOk()) {
        return false;
    }

    if (m_authenticated) {
        storage.put("auth_token", m_userInfo.token);
        storage.put("auth_username", m_userInfo.username);
        storage.put("auth_role", m_userInfo.role);
        storage.put("auth_refresh_token", m_userInfo.refreshToken);
    } else {
        storage.remove("auth_token");
        storage.remove("auth_username");
        storage.remove("auth_role");
        storage.remove("auth_refresh_token");
    }

    return true;
}

void AuthManager::notifyAuthStateChanged(bool authenticated) {
    if (m_authStateCallback) {
        m_authStateCallback(authenticated);
    }
}

}
