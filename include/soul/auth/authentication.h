#ifndef SOUL_AUTH_AUTHENTICATION_H
#define SOUL_AUTH_AUTHENTICATION_H

// ============================================================================
// authentication.h — 认证层 (Who are you?) [v2.7.0 新增]
// ============================================================================
//
// 职责: 验证用户身份 (账号密码/Token/OAuth2/OIDC)
// 输入: Credential (凭证)
// 输出: Identity (身份) 或 Error
//
// 与 Authorization 的关系:
//   Authentication → Identity → Authorization → Policy
//
// 用法:
//   auto auth = std::make_shared<PasswordAuthenticator>(userRepo, hashService);
//   auto result = auth->authenticate({"admin", "secret123"});
//   if (result.isOk()) {
//       auto identity = result.unwrap();
//       // identity.userId, identity.username, identity.roles
//   }

#include <QString>
#include <QDateTime>
#include <QVector>
#include <memory>
#include <vector>
#include <string>

#include "soul/core/result.h"

namespace sc {
namespace auth {

// ============================================================================
// Credential — 凭证 [v2.7.0]
// ============================================================================

struct Credential {
    QString type;       // "password", "token", "oauth2", "oidc", "apikey"
    QString principal;  // 用户名 / Token 字符串 / API Key
    QString secret;     // 密码 (password 模式) 或空 (token 模式)
    QMap<QString, QString> metadata;  // 额外元数据 (如 OAuth2 state)
};

// ============================================================================
// Identity — 身份 [v2.7.0]
// ============================================================================

struct Identity {
    QString userId;
    QString username;
    QString displayName;
    QVector<QString> roles;
    QDateTime authenticatedAt;
    QMap<QString, QString> attributes;  // 扩展属性
};

// ============================================================================
// IAuthenticator — 认证器接口 [v2.7.0]
// ============================================================================

class IAuthenticator {
public:
    virtual ~IAuthenticator() = default;

    /// @brief 验证凭证，返回身份或错误
    virtual Result<Identity> authenticate(const Credential& credential) = 0;

    /// @brief 验证 Token 有效性
    virtual Result<Identity> validateToken(const QString& token) = 0;

    /// @brief 刷新 Token
    virtual Result<QString> refreshToken(const QString& refreshToken) = 0;

    /// @brief 使 Token 失效 (登出)
    virtual void invalidateToken(const QString& token) = 0;

    /// @return 认证器名称
    virtual std::string name() const = 0;
};

// ============================================================================
// PasswordAuthenticator — 密码认证器 [v2.7.0]
// ============================================================================
// 基于用户名/密码的认证。
// 依赖 IUserRepository (查找用户) 和 IPasswordHasher (密码校验)。

class IUserRepository {
public:
    virtual ~IUserRepository() = default;
    virtual Result<Identity> findByUsername(const QString& username) = 0;
    virtual Result<QString> getPasswordHash(const QString& userId) = 0;
};

class IPasswordHasher {
public:
    virtual ~IPasswordHasher() = default;
    virtual QString hash(const QString& password) = 0;
    virtual bool verify(const QString& password, const QString& hash) = 0;
};

class PasswordAuthenticator : public IAuthenticator {
public:
    PasswordAuthenticator(std::shared_ptr<IUserRepository> userRepo,
                          std::shared_ptr<IPasswordHasher> hasher);

    Result<Identity> authenticate(const Credential& credential) override;
    Result<Identity> validateToken(const QString& token) override;
    Result<QString> refreshToken(const QString& refreshToken) override;
    void invalidateToken(const QString& token) override;
    std::string name() const override { return "PasswordAuthenticator"; }

private:
    std::shared_ptr<IUserRepository> m_userRepo;
    std::shared_ptr<IPasswordHasher> m_hasher;
};

// ============================================================================
// TokenAuthenticator — Token 认证器 [v2.7.0]
// ============================================================================

class ITokenProvider {
public:
    virtual ~ITokenProvider() = default;
    virtual Result<Identity> parseToken(const QString& token) = 0;
    virtual bool isTokenValid(const QString& token) = 0;
};

class TokenAuthenticator : public IAuthenticator {
public:
    explicit TokenAuthenticator(std::shared_ptr<ITokenProvider> tokenProvider);

    Result<Identity> authenticate(const Credential& credential) override;
    Result<Identity> validateToken(const QString& token) override;
    Result<QString> refreshToken(const QString& refreshToken) override;
    void invalidateToken(const QString& token) override;
    std::string name() const override { return "TokenAuthenticator"; }

private:
    std::shared_ptr<ITokenProvider> m_tokenProvider;
};

} // namespace auth
} // namespace sc

#endif // SOUL_AUTH_AUTHENTICATION_H
