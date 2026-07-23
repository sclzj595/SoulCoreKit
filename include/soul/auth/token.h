#ifndef SOUL_AUTH_TOKEN_H
#define SOUL_AUTH_TOKEN_H

#include <QString>
#include <QDateTime>

namespace sc {

/**
 * @class Token
 * @brief Token 管理类
 *
 * Token 封装了认证令牌的信息，包括：
 * - Access Token
 * - Refresh Token
 * - Token 类型
 * - 过期时间
 * - Token 验证和刷新
 *
 * 使用方式：
 * @code
 * Token token("Bearer", "access_token", "refresh_token", QDateTime::currentDateTime().addHours(1));
 * if (token.isExpired()) {
 *     // 需要刷新 Token
 * }
 * QString authHeader = token.authorizationHeader();
 * @endcode
 *
 * @see AuthManager, User
 */
class Token {
public:
    /**
     * @brief 默认构造函数
     */
    Token() = default;

    /**
     * @brief 构造函数
     * @param tokenType Token 类型（如 "Bearer"）
     * @param accessToken Access Token
     * @param refreshToken Refresh Token
     * @param expiresAt 过期时间
     */
    Token(const QString& tokenType, const QString& accessToken, 
          const QString& refreshToken = "", 
          const QDateTime& expiresAt = QDateTime());

    /**
     * @brief 获取 Token 类型
     * @return Token 类型
     */
    QString tokenType() const;

    /**
     * @brief 设置 Token 类型
     * @param tokenType Token 类型
     */
    void setTokenType(const QString& tokenType);

    /**
     * @brief 获取 Access Token
     * @return Access Token
     */
    QString accessToken() const;

    /**
     * @brief 设置 Access Token
     * @param accessToken Access Token
     */
    void setAccessToken(const QString& accessToken);

    /**
     * @brief 获取 Refresh Token
     * @return Refresh Token
     */
    QString refreshToken() const;

    /**
     * @brief 设置 Refresh Token
     * @param refreshToken Refresh Token
     */
    void setRefreshToken(const QString& refreshToken);

    /**
     * @brief 获取过期时间
     * @return 过期时间
     */
    QDateTime expiresAt() const;

    /**
     * @brief 设置过期时间
     * @param expiresAt 过期时间
     */
    void setExpiresAt(const QDateTime& expiresAt);

    /**
     * @brief 检查 Token 是否为空
     * @return 如果为空返回 true
     */
    bool isEmpty() const;

    /**
     * @brief 检查 Token 是否过期
     * @return 如果过期返回 true
     */
    bool isExpired() const;

    /**
     * @brief 检查 Token 是否即将过期（默认提前60秒）
     * @param seconds 提前检查的秒数
     * @return 如果即将过期返回 true
     */
    bool isAboutToExpire(int seconds = 60) const;

    /**
     * @brief 获取剩余有效期（秒）
     * @return 剩余秒数，如果已过期返回负数
     */
    int remainingSeconds() const;

    /**
     * @brief 生成 Authorization 请求头
     * @return Authorization 头值（如 "Bearer xxx"）
     */
    QString authorizationHeader() const;

    /**
     * @brief 清空 Token
     */
    void clear();

    /**
     * @brief 转换为 QVariantMap
     * @return QVariantMap
     */
    QVariantMap toMap() const;

    /**
     * @brief 从 QVariantMap 创建 Token
     * @param map QVariantMap
     * @return Token 对象
     */
    static Token fromMap(const QVariantMap& map);

private:
    QString m_tokenType;
    QString m_accessToken;
    QString m_refreshToken;
    QDateTime m_expiresAt;
};

}

#endif
