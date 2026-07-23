#ifndef SOUL_AUTH_TOKEN_MANAGER_H
#define SOUL_AUTH_TOKEN_MANAGER_H

#include <QString>
#include <memory>
#include <QJsonObject>
#include "soul/core/result.h"
#include "soul/base/base_manager.h"

namespace sc {

/**
 * @class TokenManager
 * @brief Token 管理器，处理 Token 的存储、刷新和验证
 *
 * TokenManager 负责：
 * - Token 的安全存储（内存 + 文件持久化）
 * - Token 过期检查
 * - Token 刷新逻辑
 * - Token 格式验证
 *
 * 使用方式：
 * @code
 * TokenManager tokenManager;
 * tokenManager.init();
 * tokenManager.setToken("Bearer xxx");
 * tokenManager.setRefreshToken("refresh_token");
 *
 * if (tokenManager.isTokenExpired()) {
 *     auto result = tokenManager.refresh();
 * }
 *
 * QString token = tokenManager.token();
 *
 * tokenManager.shutdown();
 * @endcode
 *
 * @see AuthManager
 */
class TokenManager : public BaseManager {
public:
    /**
     * @brief Token 刷新回调类型
     */
    using RefreshCallback = std::function<Result<QString>(const QString& refreshToken)>;

    /**
     * @brief 构造函数
     */
    TokenManager();

    /**
     * @brief 析构函数
     */
    ~TokenManager() = default;

    /**
     * @brief 设置访问 Token
     * @param token Token 字符串
     */
    void setToken(const QString& token);

    /**
     * @brief 获取访问 Token
     * @return Token 字符串
     */
    QString token() const;

    /**
     * @brief 设置 Refresh Token
     * @param refreshToken Refresh Token 字符串
     */
    void setRefreshToken(const QString& refreshToken);

    /**
     * @brief 获取 Refresh Token
     * @return Refresh Token 字符串
     */
    QString refreshToken() const;

    /**
     * @brief 设置 Token 过期时间（秒）
     * @param expiresIn 过期时间
     */
    void setExpiresIn(int expiresIn);

    /**
     * @brief 获取 Token 过期时间
     * @return 过期时间（秒）
     */
    int expiresIn() const;

    /**
     * @brief 设置 Token 刷新回调
     * @param callback 刷新回调函数
     */
    void setRefreshCallback(RefreshCallback callback);

    /**
     * @brief 检查 Token 是否过期
     * @return 如果已过期返回 true
     */
    bool isTokenExpired() const;

    /**
     * @brief 检查 Token 是否即将过期（剩余时间小于阈值）
     * @param thresholdSeconds 阈值（秒），默认 300 秒（5分钟）
     * @return 如果即将过期返回 true
     */
    bool isTokenAboutToExpire(int thresholdSeconds = 300) const;

    /**
     * @brief 刷新 Token
     * @return 包含新 Token 的 Result
     */
    Result<QString> refresh();

    /**
     * @brief 当 Token 即将过期时自动刷新
     * @param thresholdSeconds 阈值（秒），默认 300 秒（5分钟）
     * @return 包含新 Token 或当前 Token 的 Result
     */
    Result<QString> autoRefreshIfNeeded(int thresholdSeconds = 300);

    /**
     * @brief 清空所有 Token
     */
    void clear();

    /**
     * @brief 从 Token 中提取用户 ID（如果 Token 是 JWT 格式）
     * @return 用户 ID，如果无法提取返回空字符串
     */
    QString extractUserId() const;

    /**
     * @brief 验证 Token 格式是否有效
     * @param token Token 字符串
     * @return 如果格式有效返回 true
     */
    static bool validateTokenFormat(const QString& token);

    bool init() override;
    void shutdown() override;

private:
    QString m_token;
    QString m_refreshToken;
    int m_expiresIn = 0;
    qint64 m_tokenIssueTime = 0;
    RefreshCallback m_refreshCallback;
    QJsonObject m_jwtPayload;

    QJsonObject parseJwtPayload() const;
    void doParseJwtPayload();
};

}

#endif
