/**
 * @file http_response.h
 * @brief HTTP 响应类
 * @details 封装 HTTP 响应的状态码、响应体、响应头和错误信息，提供 JSON 和文本解析
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef SOUL_NETWORK_HTTP_RESPONSE_H
#define SOUL_NETWORK_HTTP_RESPONSE_H

#include "soul/network/network_global.h"
#include <QMap>
#include <QByteArray>
#include <QJsonDocument>
#include <QNetworkReply>

namespace sc {
namespace network {

class SC_NETWORK_EXPORT HttpResponse {
public:
    /**
     * @brief 默认构造函数
     */
    HttpResponse();

    /**
     * @brief 获取 HTTP 状态码
     * @return 状态码
     */
    int statusCode() const;

    /**
     * @brief 获取响应体
     * @return 响应体数据
     */
    QByteArray body() const;

    /**
     * @brief 获取响应头
     * @return 响应头映射
     */
    QMap<QString, QString> headers() const;

    /**
     * @brief 获取 JSON 响应
     * @return JSON 文档
     */
    QJsonDocument json() const;

    /**
     * @brief 获取文本响应
     * @return 响应文本
     */
    QString text() const;

    /**
     * @brief 检查响应是否成功（状态码 200-299）
     * @return 如果成功返回 true
     */
    bool isSuccess() const;

    /**
     * @brief 检查响应是否出错
     * @return 如果出错返回 true
     */
    bool isError() const;

    /**
     * @brief 设置 HTTP 状态码
     * @param code 状态码
     */
    void setStatusCode(int code);

    /**
     * @brief 设置响应体
     * @param body 响应体数据
     */
    void setBody(const QByteArray& body);

    /**
     * @brief 设置响应头
     * @param headers 响应头映射
     */
    void setHeaders(const QMap<QString, QString>& headers);

    /**
     * @brief 设置网络错误
     * @param error 网络错误类型
     * @param errorString 错误描述
     */
    void setError(QNetworkReply::NetworkError error, const QString& errorString);

    /**
     * @brief 获取网络错误类型
     * @return 网络错误类型
     */
    QNetworkReply::NetworkError networkError() const;

    /**
     * @brief 获取错误描述
     * @return 错误描述字符串
     */
    QString errorString() const;

private:
    int m_statusCode = 0;
    QByteArray m_body;
    QMap<QString, QString> m_headers;
    QNetworkReply::NetworkError m_networkError = QNetworkReply::NoError;
    QString m_errorString;
};

} // namespace network
} // namespace sc

#endif
