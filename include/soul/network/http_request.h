#ifndef SOUL_NETWORK_HTTP_REQUEST_H
#define SOUL_NETWORK_HTTP_REQUEST_H

#include <QUrl>
#include <QMap>
#include <QByteArray>
#include <QJsonDocument>

namespace sc {

/**
 * @enum HttpMethod
 * @brief HTTP 请求方法枚举
 */
enum class HttpMethod {
    Get,     ///< GET 请求
    Post,    ///< POST 请求
    Put,     ///< PUT 请求
    Delete,  ///< DELETE 请求
    Patch,   ///< PATCH 请求
    Head,    ///< HEAD 请求
    Options, ///< OPTIONS 请求
};

/**
 * @class HttpRequest
 * @brief HTTP 请求封装类
 *
 * HttpRequest 提供了链式 API 来构建 HTTP 请求，包括设置请求方法、URL、
 * 请求头、查询参数和请求体。
 *
 * 使用方式：
 * @code
 * HttpRequest request(HttpMethod::Get, QUrl("https://api.example.com/users"));
 * request.addParam("page", "1")
 *        .addParam("limit", "10")
 *        .addHeader("Authorization", "Bearer token");
 * @endcode
 *
 * @see HttpClient, HttpResponse
 */
class HttpRequest {
public:
    /**
     * @brief 默认构造函数
     */
    HttpRequest();

    /**
     * @brief 构造函数
     * @param method HTTP 请求方法
     * @param url 请求 URL
     */
    HttpRequest(HttpMethod method, const QUrl& url);

    /**
     * @brief 获取请求方法
     * @return HTTP 请求方法
     */
    HttpMethod method() const;

    /**
     * @brief 获取请求 URL
     * @return 请求 URL
     */
    QUrl url() const;

    /**
     * @brief 设置请求方法
     * @param method HTTP 请求方法
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& setMethod(HttpMethod method);

    /**
     * @brief 设置请求 URL
     * @param url 请求 URL
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& setUrl(const QUrl& url);

    /**
     * @brief 添加请求头
     * @param key 请求头名称
     * @param value 请求头值
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& addHeader(const QString& key, const QString& value);

    /**
     * @brief 设置请求头
     * @param headers 请求头映射
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& setHeaders(const QMap<QString, QString>& headers);

    /**
     * @brief 获取请求头
     * @return 请求头映射
     */
    QMap<QString, QString> headers() const;

    /**
     * @brief 添加查询参数
     * @param key 参数名称
     * @param value 参数值
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& addParam(const QString& key, const QString& value);

    /**
     * @brief 设置查询参数
     * @param params 查询参数映射
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& setParams(const QMap<QString, QString>& params);

    /**
     * @brief 获取查询参数
     * @return 查询参数映射
     */
    QMap<QString, QString> params() const;

    /**
     * @brief 设置请求体
     * @param body 请求体数据
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& setBody(const QByteArray& body);

    /**
     * @brief 设置 JSON 请求体
     * @param json JSON 文档
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& setJsonBody(const QJsonDocument& json);

    /**
     * @brief 获取请求体
     * @return 请求体数据
     */
    QByteArray body() const;

    /**
     * @brief 设置超时时间
     * @param ms 超时时间（毫秒）
     * @return 当前 HttpRequest 对象引用（链式调用）
     */
    HttpRequest& setTimeout(int ms);

    /**
     * @brief 获取超时时间
     * @return 超时时间（毫秒）
     */
    int timeout() const;

    /**
     * @brief 构建完整 URL（自动拼接查询参数）
     * @return 包含查询参数的完整 URL
     */
    QUrl buildUrl() const;

private:
    HttpMethod m_method = HttpMethod::Get;
    QUrl m_url;
    QMap<QString, QString> m_headers;
    QMap<QString, QString> m_params;
    QByteArray m_body;
    int m_timeout = 30000;
};

}

#endif
