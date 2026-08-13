#include "soul/auth/oauth2.h"

#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QRandomGenerator>
#include <QCryptographicHash>
#include <QTimer>

#include "soul/utils/json/json_helper.h"

namespace sc::auth {

using namespace sc::json;

// ============================================================================
// 内部辅助函数
// ============================================================================

namespace {

/**
 * @brief 将 QByteArray 转换为 Base64URL 编码（无填充）
 */
QByteArray base64UrlEncode(const QByteArray& data) {
    QByteArray encoded = data.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    return encoded;
}

/**
 * @brief 解析 OAuth2 Token 端点的 JSON 响应
 */
OAuth2Token parseTokenResponse(const Json& json) {
    OAuth2Token token;
    token.accessToken = getString(json, "access_token");
    token.refreshToken = getString(json, "refresh_token");
    token.tokenType = getString(json, "token_type");
    token.expiresIn = static_cast<int>(getInt64(json, "expires_in"));
    token.idToken = getString(json, "id_token");
    return token;
}

/**
 * @brief 同步 HTTP POST 请求（使用 QEventLoop 阻塞等待）
 */
Result<Json> httpPostSync(const QUrl& url, const QByteArray& body,
                           const QString& contentType = QStringLiteral("application/x-www-form-urlencoded")) {
    QNetworkAccessManager nam;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    request.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = nam.post(request, body);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
    timer.start(30000); // 30 秒超时

    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        reply->deleteLater();
        return Error(ErrorCode::Timeout, "OAuth2 token request timed out");
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();

        auto parseResult = deserialize(responseData);
        QString errorMsg;
        if (parseResult.isOk()) {
            Json obj = parseResult.unwrap();
            errorMsg = getString(obj, "error_description");
            if (errorMsg.isEmpty()) {
                errorMsg = getString(obj, "error");
            }
        }
        if (errorMsg.isEmpty()) {
            errorMsg = reply->errorString();
        }
        reply->deleteLater();
        return Error(ErrorCode::NetworkError, errorMsg);
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    return deserialize(responseData);
}

/**
 * @brief 同步 HTTP GET 请求
 */
[[maybe_unused]] static Result<Json> httpGetSync(const QUrl& url) {
    QNetworkAccessManager nam;
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");

    QNetworkReply* reply = nam.get(request);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::errorOccurred, &loop, &QEventLoop::quit);
    timer.start(15000); // 15 秒超时

    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        reply->deleteLater();
        return Error(ErrorCode::Timeout, "HTTP GET request timed out");
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        QString errorMsg = reply->errorString();
        reply->deleteLater();
        return Error(ErrorCode::NetworkError, errorMsg);
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    return deserialize(responseData);
}

} // anonymous namespace

// ============================================================================
// AuthorizationCodeFlow 实现
// ============================================================================

AuthorizationCodeFlow::AuthorizationCodeFlow(const OAuth2Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
    m_codeVerifier = generateCodeVerifier();
    m_state = generateState();  // [v1.9.2] 生成 CSRF state 参数
}

QString AuthorizationCodeFlow::codeVerifier() const {
    return m_codeVerifier;
}

QString AuthorizationCodeFlow::state() const {
    return m_state;
}

bool AuthorizationCodeFlow::validateState(const QString& returnedState) const {
    return !m_state.isEmpty() && m_state == returnedState;
}

QString AuthorizationCodeFlow::generateState() const {
    // 生成 16 字节随机 state(32 字符 hex)
    QByteArray randomBytes(16, '\0');
    QRandomGenerator* rng = QRandomGenerator::global();
    for (int i = 0; i < randomBytes.size(); ++i) {
        randomBytes[i] = static_cast<char>(rng->bounded(256));
    }
    return QString::fromLatin1(randomBytes.toHex());
}

QString AuthorizationCodeFlow::generateCodeVerifier() const {
    // 生成 32 字节随机数据（256 位熵）
    QByteArray randomBytes(32, '\0');
    QRandomGenerator* rng = QRandomGenerator::global();
    for (int i = 0; i < randomBytes.size(); ++i) {
        randomBytes[i] = static_cast<char>(rng->bounded(256));
    }
    return QString::fromLatin1(base64UrlEncode(randomBytes));
}

QString AuthorizationCodeFlow::generateCodeChallenge(const QString& verifier) const {
    QByteArray hash = QCryptographicHash::hash(verifier.toUtf8(), QCryptographicHash::Sha256);
    return QString::fromLatin1(base64UrlEncode(hash));
}

QString AuthorizationCodeFlow::authorizationUrl() const {
    QUrl url(m_config.authorizationEndpoint);
    QUrlQuery query;

    query.addQueryItem("response_type", "code");
    query.addQueryItem("client_id", m_config.clientId);
    query.addQueryItem("redirect_uri", m_config.redirectUri);

    if (!m_config.scopes.isEmpty()) {
        query.addQueryItem("scope", m_config.scopes.join(" "));
    }

    if (m_config.usePkce) {
        QString challenge = generateCodeChallenge(m_codeVerifier);
        query.addQueryItem("code_challenge", challenge);
        query.addQueryItem("code_challenge_method", "S256");
    }

    // [v1.9.2] 添加 state 参数防 CSRF
    query.addQueryItem("state", m_state);

    url.setQuery(query);
    return url.toString();
}

Result<OAuth2Token> AuthorizationCodeFlow::exchangeCodeForToken(const QString& code, const QString& codeVerifier) {
    if (code.isEmpty()) {
        return Error(ErrorCode::InvalidArgument, "Authorization code is empty");
    }

    QUrl url(m_config.tokenEndpoint);
    QUrlQuery body;

    body.addQueryItem("grant_type", "authorization_code");
    body.addQueryItem("code", code);
    body.addQueryItem("client_id", m_config.clientId);
    body.addQueryItem("redirect_uri", m_config.redirectUri);

    if (m_config.usePkce && !codeVerifier.isEmpty()) {
        body.addQueryItem("code_verifier", codeVerifier);
    }

    auto result = httpPostSync(url, body.toString().toUtf8());
    if (!result.isOk()) {
        return result.unwrapErr();
    }

    OAuth2Token token = parseTokenResponse(result.unwrap());

    if (token.accessToken.isEmpty()) {
        return Error(ErrorCode::AuthError, "Token response missing access_token");
    }

    emit tokenReceived(token);
    return Result<OAuth2Token>(token);
}

Result<OAuth2Token> AuthorizationCodeFlow::refreshToken(const QString& refreshToken) {
    if (refreshToken.isEmpty()) {
        return Error(ErrorCode::InvalidArgument, "Refresh token is empty");
    }

    QUrl url(m_config.tokenEndpoint);
    QUrlQuery body;

    body.addQueryItem("grant_type", "refresh_token");
    body.addQueryItem("refresh_token", refreshToken);
    body.addQueryItem("client_id", m_config.clientId);

    if (!m_config.clientSecret.isEmpty()) {
        body.addQueryItem("client_secret", m_config.clientSecret);
    }

    auto result = httpPostSync(url, body.toString().toUtf8());
    if (!result.isOk()) {
        return result.unwrapErr();
    }

    OAuth2Token token = parseTokenResponse(result.unwrap());

    if (token.accessToken.isEmpty()) {
        return Error(ErrorCode::AuthError, "Token response missing access_token");
    }

    emit tokenReceived(token);
    return Result<OAuth2Token>(token);
}

// ============================================================================
// ClientCredentialsFlow 实现
// ============================================================================

ClientCredentialsFlow::ClientCredentialsFlow(const OAuth2Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
}

Result<OAuth2Token> ClientCredentialsFlow::requestToken() {
    QUrl url(m_config.tokenEndpoint);
    QUrlQuery body;

    body.addQueryItem("grant_type", "client_credentials");
    body.addQueryItem("client_id", m_config.clientId);
    body.addQueryItem("client_secret", m_config.clientSecret);

    if (!m_config.scopes.isEmpty()) {
        body.addQueryItem("scope", m_config.scopes.join(" "));
    }

    auto result = httpPostSync(url, body.toString().toUtf8());
    if (!result.isOk()) {
        return result.unwrapErr();
    }

    OAuth2Token token = parseTokenResponse(result.unwrap());

    if (token.accessToken.isEmpty()) {
        return Error(ErrorCode::AuthError, "Token response missing access_token");
    }

    emit tokenReceived(token);
    return Result<OAuth2Token>(token);
}

} // namespace sc::auth
