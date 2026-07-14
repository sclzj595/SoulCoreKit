#ifndef SOUL_NETWORK_SESSION_H
#define SOUL_NETWORK_SESSION_H

#include <QString>
#include <QMap>
#include <memory>

namespace sc {

/**
 * @class Session
 * @brief 会话管理类，管理用户登录状态和Token
 *
 * Session 采用单例模式，用于管理用户登录后的会话状态，包括：
 * - 认证 Token 管理
 * - 会话数据存储
 * - 会话清理
 *
 * 使用方式：
 * @code
 * Session::instance().setToken("Bearer xxx");
 * QString token = Session::instance().token();
 * Session::instance().set("username", "user123");
 * @endcode
 *
 * @see AuthManager
 */
class Session {
public:
    /**
     * @brief 获取单例实例
     * @return 单例对象引用
     */
    static Session& instance();

    /**
     * @brief 获取认证 Token
     * @return Token 字符串
     */
    QString token() const;

    /**
     * @brief 设置认证 Token
     * @param token Token 字符串
     */
    void setToken(const QString& token);

    /**
     * @brief 获取会话数据
     * @param key 数据键
     * @return 数据值，如果不存在返回空字符串
     */
    QString get(const QString& key) const;

    /**
     * @brief 设置会话数据
     * @param key 数据键
     * @param value 数据值
     */
    void set(const QString& key, const QString& value);

    /**
     * @brief 清空所有会话数据
     */
    void clear();

    /**
     * @brief 检查会话是否为空
     * @return 如果为空返回 true
     */
    bool isEmpty() const;

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Session() = default;

    QString m_token;
    QMap<QString, QString> m_data;
};

}

#endif
