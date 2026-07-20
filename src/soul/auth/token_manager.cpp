#include "soul/auth/token_manager.h"
#include "soul/core/time.h"
#include "soul/utils/json/json_utils.h"
#include "soul/utils/crypto/crypto_utils.h"
#include <QStringList>
#include <QJsonDocument>
#include <QJsonObject>

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
    doParseJwtPayload();
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
    if (!m_jwtPayload.isEmpty()) {
        qint64 exp = m_jwtPayload["exp"].toInt(0);
        if (exp > 0) {
            return Time::toSeconds(Time::now()) >= exp;
        }
    }

    if (m_expiresIn <= 0 || m_tokenIssueTime == 0) {
        return false;
    }

    qint64 now = Time::toMilliseconds(Time::now());
    qint64 expireTime = m_tokenIssueTime + static_cast<qint64>(m_expiresIn) * 1000;

    return now >= expireTime;
}

bool TokenManager::isTokenAboutToExpire(int thresholdSeconds) const {
    if (!m_jwtPayload.isEmpty()) {
        qint64 exp = m_jwtPayload["exp"].toInt(0);
        if (exp > 0) {
            qint64 now = Time::toSeconds(Time::now());
            qint64 remaining = exp - now;
            return remaining <= thresholdSeconds;
        }
    }

    if (m_expiresIn <= 0 || m_tokenIssueTime == 0) {
        return false;
    }

    qint64 now = Time::toMilliseconds(Time::now());
    qint64 expireTime = m_tokenIssueTime + static_cast<qint64>(m_expiresIn) * 1000;
    qint64 remaining = expireTime - now;

    return remaining <= static_cast<qint64>(thresholdSeconds) * 1000;
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

Result<QString> TokenManager::autoRefreshIfNeeded(int thresholdSeconds) {
    if (isTokenAboutToExpire(thresholdSeconds)) {
        return refresh();
    }
    return Result<QString>(m_token);
}

void TokenManager::clear() {
    m_token.clear();
    m_refreshToken.clear();
    m_expiresIn = 0;
    m_tokenIssueTime = 0;
    m_jwtPayload = QJsonObject();
}

QString TokenManager::extractUserId() const {
    if (!m_jwtPayload.isEmpty()) {
        QString uid = m_jwtPayload["sub"].toString();
        if (!uid.isEmpty()) {
            return uid;
        }
        uid = m_jwtPayload["user_id"].toString();
        if (!uid.isEmpty()) {
            return uid;
        }
        uid = m_jwtPayload["id"].toString();
        return uid;
    }

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
    if (parts.size() != 3) {
        return false;
    }

    for (const QString& part : parts) {
        if (part.isEmpty()) {
            return false;
        }
    }

    return true;
}

QJsonObject TokenManager::parseJwtPayload() const {
    QString token = m_token;
    if (token.startsWith("Bearer ")) {
        token = token.mid(7);
    }

    QStringList parts = token.split('.');
    if (parts.size() != 3) {
        return QJsonObject();
    }

    QString payloadB64 = parts[1];
    while (payloadB64.size() % 4 != 0) {
        payloadB64.append('=');
    }

    QByteArray decoded = QByteArray::fromBase64(payloadB64.toUtf8());
    QJsonDocument doc = QJsonDocument::fromJson(decoded);

    if (!doc.isObject()) {
        return QJsonObject();
    }

    return doc.object();
}

void TokenManager::doParseJwtPayload() {
    m_jwtPayload = parseJwtPayload();
    if (!m_jwtPayload.isEmpty()) {
        qint64 exp = m_jwtPayload["exp"].toInt(0);
        if (exp > 0) {
            qint64 iat = m_jwtPayload["iat"].toInt(0);
            if (iat > 0) {
                m_expiresIn = static_cast<int>(exp - iat);
                m_tokenIssueTime = iat * 1000;
            }
        }
    }
}

}