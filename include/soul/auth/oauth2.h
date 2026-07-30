#ifndef SOUL_AUTH_OAUTH2_H
#define SOUL_AUTH_OAUTH2_H

#include <QObject>
#include <QString>
#include <QStringList>
#include "soul/core/result.h"

namespace sc::auth {

/**
 * @brief OAuth2 配置结构体
 */
struct OAuth2Config {
    QString clientId;
    QString clientSecret;
    QString authorizationEndpoint;
    QString tokenEndpoint;
    QString redirectUri;
    QStringList scopes;
    bool usePkce = true;
};

/**
 * @brief OAuth2 Token 响应结构体
 */
struct OAuth2Token {
    QString accessToken;
    QString refreshToken;
    QString tokenType;
    int expiresIn = 0;
    QString idToken; // OIDC
};

/**
 * @class AuthorizationCodeFlow
 * @brief OAuth2 授权码流程（支持 PKCE S256 + state 参数防 CSRF）
 *
 * 实现 OAuth2 Authorization Code Grant with PKCE，
 * 适用于无法安全存储 client_secret 的客户端（SPA、移动端等）。
 *
 * [v1.9.2] 新增 state 参数生成与验证，防止 CSRF 攻击:
 *   - authorizationUrl() 自动生成随机 state 并存储
 *   - exchangeCodeForToken() 验证返回的 state 与存储值一致
 *
 * 使用方式：
 * @code
 * OAuth2Config config;
 * config.clientId = "my-client";
 * config.authorizationEndpoint = "https://auth.example.com/authorize";
 * config.tokenEndpoint = "https://auth.example.com/token";
 * config.redirectUri = "myapp://callback";
 * config.scopes = {"openid", "profile"};
 *
 * AuthorizationCodeFlow flow(config);
 * QString url = flow.authorizationUrl();  // 生成授权 URL(含 state)
 * // 用户授权后获取 code 和 state
 * auto result = flow.exchangeCodeForToken(code, flow.codeVerifier());
 * @endcode
 */
class AuthorizationCodeFlow : public QObject {
    Q_OBJECT
public:
    explicit AuthorizationCodeFlow(const OAuth2Config& config, QObject* parent = nullptr);

    /**
     * @brief 生成授权 URL（包含 PKCE code_challenge 和 state 参数）
     * @return 完整的授权端点 URL
     */
    QString authorizationUrl() const;

    /**
     * @brief 使用授权码交换 Token
     * @param code 授权码
     * @param codeVerifier PKCE code_verifier
     * @return 包含 OAuth2Token 的 Result
     */
    Result<OAuth2Token> exchangeCodeForToken(const QString& code, const QString& codeVerifier);

    /**
     * @brief 获取生成的 state 参数(用于回调验证) [v1.9.2 新增]
     * @return state 值
     */
    QString state() const;

    /**
     * @brief 验证回调中的 state 参数 [v1.9.2 新增]
     * @param returnedState 回调返回的 state 值
     * @return 验证通过返回 true
     */
    bool validateState(const QString& returnedState) const;

    /**
     * @brief 使用 Refresh Token 刷新 Token
     * @param refreshToken Refresh Token
     * @return 包含新 OAuth2Token 的 Result
     */
    Result<OAuth2Token> refreshToken(const QString& refreshToken);

    /**
     * @brief 获取 PKCE Code Verifier
     * @return Code Verifier 字符串
     */
    QString codeVerifier() const;

signals:
    void tokenReceived(const OAuth2Token& token);
    void tokenError(const QString& error);

private:
    QString generateCodeVerifier() const;
    QString generateCodeChallenge(const QString& verifier) const;
    QString generateState() const;  ///< [v1.9.2] 生成随机 state 参数
    OAuth2Config m_config;
    QString m_codeVerifier;
    QString m_state;  ///< [v1.9.2] CSRF state 参数
};

/**
 * @class ClientCredentialsFlow
 * @brief OAuth2 客户端凭证流程
 *
 * 实现 OAuth2 Client Credentials Grant，
 * 适用于服务器到服务器的认证场景。
 *
 * 使用方式：
 * @code
 * OAuth2Config config;
 * config.clientId = "my-service";
 * config.clientSecret = "secret";
 * config.tokenEndpoint = "https://auth.example.com/token";
 * config.scopes = {"api:read"};
 *
 * ClientCredentialsFlow flow(config);
 * auto result = flow.requestToken();
 * @endcode
 */
class ClientCredentialsFlow : public QObject {
    Q_OBJECT
public:
    explicit ClientCredentialsFlow(const OAuth2Config& config, QObject* parent = nullptr);

    /**
     * @brief 请求 Token
     * @return 包含 OAuth2Token 的 Result
     */
    Result<OAuth2Token> requestToken();

signals:
    void tokenReceived(const OAuth2Token& token);
    void tokenError(const QString& error);

private:
    OAuth2Config m_config;
};

} // namespace sc::auth

#endif