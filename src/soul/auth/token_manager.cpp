#include "soul/auth/token_manager.h"
#include "soul/core/time.h"
#include "soul/utils/json/json_helper.h"
#include "soul/utils/crypto/crypto_utils.h"
#include <QStringList>

namespace sc {

using namespace sc::json;

TokenManager::TokenManager() {
}

TokenManager::TokenManager(std::shared_ptr<auth::ISecureStorage> storage)
    : m_secureStorage(std::move(storage)) {
}

Result<void> TokenManager::init() {
    m_initialized = true;
    return Ok();
}

void TokenManager::shutdown() {
    // [v1.9.2] 关闭前持久化 Token
    if (m_secureStorage) {
        (void)persistToken();
    }
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
    if (!m_jwtPayload.empty()) {
        qint64 exp = m_jwtPayload.value("exp", 0);
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
    if (!m_jwtPayload.empty()) {
        qint64 exp = m_jwtPayload.value("exp", 0);
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
    m_jwtPayload = Json::object();
}

QString TokenManager::extractUserId() const {
    if (!m_jwtPayload.empty()) {
        QString uid = getString(m_jwtPayload, "sub");
        if (!uid.isEmpty()) {
            return uid;
        }
        uid = getString(m_jwtPayload, "user_id");
        if (!uid.isEmpty()) {
            return uid;
        }
        uid = getString(m_jwtPayload, "id");
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

Json TokenManager::parseJwtPayload() const {
    QString token = m_token;
    if (token.startsWith("Bearer ")) {
        token = token.mid(7);
    }

    QStringList parts = token.split('.');
    if (parts.size() != 3) {
        return Json::object();
    }

    QString payloadB64 = parts[1];
    while (payloadB64.size() % 4 != 0) {
        payloadB64.append('=');
    }

    QByteArray decoded = QByteArray::fromBase64(payloadB64.toUtf8());
    auto result = deserialize(decoded);

    if (!result.isOk() || !result.unwrap().is_object()) {
        return Json::object();
    }

    return result.unwrap();
}

void TokenManager::doParseJwtPayload() {
    m_jwtPayload = parseJwtPayload();
    if (!m_jwtPayload.empty()) {
        qint64 exp = m_jwtPayload.value("exp", 0);
        if (exp > 0) {
            qint64 iat = m_jwtPayload.value("iat", 0);
            if (iat > 0) {
                m_expiresIn = static_cast<int>(exp - iat);
                m_tokenIssueTime = iat * 1000;
            }
        }
    }
}

// ============================================================================
// SecureStorage 集成 [v1.9.2 新增]
// ============================================================================

void TokenManager::setSecureStorage(std::shared_ptr<auth::ISecureStorage> storage) {
    m_secureStorage = std::move(storage);
}

Result<void> TokenManager::persistToken() {
    if (!m_secureStorage) {
        return Error(ErrorCode::InvalidState, "SecureStorage not set");
    }

    if (!m_token.isEmpty()) {
        auto result = m_secureStorage->store("access_token", m_token);
        if (!result.isOk()) return result.unwrapErr();
    }

    if (!m_refreshToken.isEmpty()) {
        auto result = m_secureStorage->store("refresh_token", m_refreshToken);
        if (!result.isOk()) return result.unwrapErr();
    }

    return {};
}

Result<void> TokenManager::restoreToken() {
    if (!m_secureStorage) {
        return Error(ErrorCode::InvalidState, "SecureStorage not set");
    }

    auto accessResult = m_secureStorage->retrieve("access_token");
    if (accessResult.isOk()) {
        setToken(accessResult.unwrap());
    }

    auto refreshResult = m_secureStorage->retrieve("refresh_token");
    if (refreshResult.isOk()) {
        m_refreshToken = refreshResult.unwrap();
    }

    return {};
}

}