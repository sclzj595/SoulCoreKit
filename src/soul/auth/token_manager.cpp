#include "soul/auth/token_manager.h"
#include "soul/core/time.h"
#include <QStringList>

namespace sc {

TokenManager::TokenManager() {
}

bool TokenManager::init() {
    m_initialized = true;
    return true;
}

void TokenManager::shutdown() {
    clear();
    m_initialized = false;
}

void TokenManager::setToken(const QString& token) {
    m_token = token;
    m_tokenIssueTime = Time::toMilliseconds(Time::now());
}

QString TokenManager::token() const {
    return m_token;
}

void TokenManager::setRefreshToken(const QString& refreshToken) {
    m_refreshToken = refreshToken;
}

QString TokenManager::refreshToken() const {
    return m_refreshToken;
}

void TokenManager::setExpiresIn(int expiresIn) {
    m_expiresIn = expiresIn;
}

int TokenManager::expiresIn() const {
    return m_expiresIn;
}

void TokenManager::setRefreshCallback(RefreshCallback callback) {
    m_refreshCallback = callback;
}

bool TokenManager::isTokenExpired() const {
    if (m_expiresIn <= 0 || m_tokenIssueTime == 0) {
        return false;
    }

    qint64 now = Time::toMilliseconds(Time::now());
    qint64 expireTime = m_tokenIssueTime + (qint64)m_expiresIn * 1000;

    return now >= expireTime;
}

bool TokenManager::isTokenAboutToExpire(int thresholdSeconds) const {
    if (m_expiresIn <= 0 || m_tokenIssueTime == 0) {
        return false;
    }

    qint64 now = Time::toMilliseconds(Time::now());
    qint64 expireTime = m_tokenIssueTime + (qint64)m_expiresIn * 1000;
    qint64 remaining = expireTime - now;

    return remaining <= (qint64)thresholdSeconds * 1000;
}

Result<QString> TokenManager::refresh() {
    if (m_refreshToken.isEmpty()) {
        return Result<QString>(Error(ErrorCode::AuthError, "No refresh token available"));
    }

    if (!m_refreshCallback) {
        return Result<QString>(Error(ErrorCode::AuthError, "Refresh callback not set"));
    }

    auto result = m_refreshCallback(m_refreshToken);
    if (result.isOk()) {
        setToken(result.unwrap());
    }

    return result;
}

void TokenManager::clear() {
    m_token.clear();
    m_refreshToken.clear();
    m_expiresIn = 0;
    m_tokenIssueTime = 0;
}

QString TokenManager::extractUserId() const {
    QString token = m_token;
    if (token.startsWith("Bearer ")) {
        token = token.mid(7);
    }

    QStringList parts = token.split('.');
    if (parts.size() >= 2) {
        return parts[1];
    }

    return QString();
}

bool TokenManager::validateTokenFormat(const QString& token) {
    if (token.isEmpty()) {
        return false;
    }

    QStringList parts = token.split('.');
    return parts.size() == 3;
}

}
