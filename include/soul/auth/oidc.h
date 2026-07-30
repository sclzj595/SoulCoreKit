#ifndef SOUL_AUTH_OIDC_H
#define SOUL_AUTH_OIDC_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "soul/core/result.h"
#include "soul/utils/json/json_helper.h"

namespace sc::auth {

/**
 * @brief OIDC Discovery 信息结构体
 *
 * 包含 OpenID Connect Discovery 1.0 规范中定义的端点信息。
 */
struct OidcDiscovery {
    QString issuer;
    QString authorizationEndpoint;
    QString tokenEndpoint;
    QString userinfoEndpoint;
    QString jwksUri;
    QStringList scopesSupported;
    QStringList responseTypesSupported;
};

/**
 * @class OidcDiscoveryService
 * @brief OIDC Discovery 服务
 *
 * 实现 OpenID Connect Discovery 1.0 协议，
 * 从 issuer URL 的 .well-known/openid-configuration 端点获取配置。
 *
 * 使用方式：
 * @code
 * OidcDiscoveryService service;
 * auto result = service.discover("https://accounts.google.com");
 * if (result.isOk()) {
 *     auto discovery = result.unwrap();
 *     // 使用 discovery.tokenEndpoint 等
 * }
 * @endcode
 */
class OidcDiscoveryService : public QObject {
    Q_OBJECT
public:
    explicit OidcDiscoveryService(QObject* parent = nullptr);

    /**
     * @brief 从 issuer URL 发现 OIDC 配置
     * @param issuerUrl OIDC Provider 的 issuer URL
     * @return 包含 OidcDiscovery 的 Result
     */
    Result<OidcDiscovery> discover(const QString& issuerUrl);

    /**
     * @brief 获取 JWKS（JSON Web Key Set）
     * @param jwksUri JWKS 端点 URL
     * @return 包含 JWKS JSON 对象的 Result
     */
    Result<sc::json::Json> fetchJwks(const QString& jwksUri);

signals:
    void discoveryCompleted(const OidcDiscovery& discovery);
    void discoveryError(const QString& error);
};

} // namespace sc::auth

#endif