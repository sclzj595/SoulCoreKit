#include "soul/auth/oidc.h"

#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QEventLoop>
#include <QTimer>

#include "soul/utils/json/json_helper.h"

namespace sc::auth {

using namespace sc::json;

// ============================================================================
// 内部辅助函数
// ============================================================================

namespace {

/**
 * @brief 同步 HTTP GET 请求（返回 Json）
 */
Result<Json> httpGetJsonSync(const QUrl& url) {
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
        return Error(ErrorCode::Timeout, "OIDC discovery request timed out");
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

/**
 * @brief 从 JSON 数组中提取字符串列表
 */
QStringList jsonArrayToStringList(const Json& arr) {
    QStringList result;
    if (arr.is_array()) {
        for (const auto& val : arr) {
            if (val.is_string()) {
                result.append(QString::fromStdString(val.get<std::string>()));
            }
        }
    }
    return result;
}

} // anonymous namespace

// ============================================================================
// OidcDiscoveryService 实现
// ============================================================================

OidcDiscoveryService::OidcDiscoveryService(QObject* parent)
    : QObject(parent)
{
}

Result<OidcDiscovery> OidcDiscoveryService::discover(const QString& issuerUrl) {
    if (issuerUrl.isEmpty()) {
        return Error(ErrorCode::InvalidArgument, "Issuer URL is empty");
    }

    // 构建 .well-known/openid-configuration URL
    QString baseUrl = issuerUrl;
    if (baseUrl.endsWith('/')) {
        baseUrl.chop(1);
    }
    QUrl discoveryUrl(baseUrl + QStringLiteral("/.well-known/openid-configuration"));

    auto result = httpGetJsonSync(discoveryUrl);
    if (!result.isOk()) {
        return result.unwrapErr();
    }

    Json json = result.unwrap();

    OidcDiscovery discovery;
    discovery.issuer = getString(json, "issuer");
    discovery.authorizationEndpoint = getString(json, "authorization_endpoint");
    discovery.tokenEndpoint = getString(json, "token_endpoint");
    discovery.userinfoEndpoint = getString(json, "userinfo_endpoint");
    discovery.jwksUri = getString(json, "jwks_uri");

    if (contains(json, "scopes_supported") && json["scopes_supported"].is_array()) {
        discovery.scopesSupported = jsonArrayToStringList(json["scopes_supported"]);
    }

    if (contains(json, "response_types_supported") && json["response_types_supported"].is_array()) {
        discovery.responseTypesSupported = jsonArrayToStringList(json["response_types_supported"]);
    }

    if (discovery.issuer.isEmpty()) {
        return Error(ErrorCode::ParseError, "OIDC discovery response missing issuer");
    }

    emit discoveryCompleted(discovery);
    return Result<OidcDiscovery>(discovery);
}

Result<sc::json::Json> OidcDiscoveryService::fetchJwks(const QString& jwksUri) {
    if (jwksUri.isEmpty()) {
        return Error(ErrorCode::InvalidArgument, "JWKS URI is empty");
    }

    QUrl url(jwksUri);
    auto result = httpGetJsonSync(url);
    if (!result.isOk()) {
        return result.unwrapErr();
    }

    Json jwks = result.unwrap();
    if (!contains(jwks, "keys")) {
        return Error(ErrorCode::ParseError, "JWKS response missing 'keys' array");
    }

    return Result<Json>(jwks);
}

} // namespace sc::auth