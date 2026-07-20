#ifndef SOUL_NETWORK_HTTP_API_H
#define SOUL_NETWORK_HTTP_API_H

#include "soul/network/http_client.h"
#include "soul/network/http_request.h"
#include "soul/network/http_response.h"
#include "soul/core/result.h"
#include <functional>
#include <memory>

namespace sc {

/**
 * @class HttpApi
 * @brief 业务 API 接口层，提供链式调用的 HTTP 请求封装
 *
 * HttpApi 是泛型模板类，用于封装业务 API 的调用，提供：
 * - 链式调用风格的 API 调用
 * - GET/POST/PUT/DELETE 方法支持
 * - 参数和请求头设置
 * - JSON 请求体支持
 * - 异步回调（成功/失败）
 *
 * 使用方式：
 * @code
 * auto api = HttpApi<MyEndpoint>(client);
 * api.get("/users")
 *    .param("page", 1)
 *    .param("limit", 10)
 *    .header("Authorization", "Bearer token")
 *    .onSuccess([](const QJsonDocument& data) {
 *        // 处理成功响应
 *    })
 *    .onFailure([](const Error& error) {
 *        // 处理失败
 *    })
 *    .execute();
 * @endcode
 *
 * @tparam Endpoint 端点类型（用于类型安全的 API 定义）
 * @see HttpClient, HttpRequest
 */
template<typename Endpoint>
class HttpApi {
public:
    /**
     * @brief 构造函数
     * @param client HTTP 客户端
     */
    HttpApi(std::shared_ptr<HttpClient> client) : m_client(client) {}

    /**
     * @brief 设置 GET 请求
     * @param path 请求路径
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& get(const QString& path) {
        m_request.setMethod(HttpMethod::Get);
        m_request.setUrl(QUrl(path));
        return *this;
    }

    /**
     * @brief 设置 POST 请求
     * @param path 请求路径
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& post(const QString& path) {
        m_request.setMethod(HttpMethod::Post);
        m_request.setUrl(QUrl(path));
        return *this;
    }

    /**
     * @brief 设置 PUT 请求
     * @param path 请求路径
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& put(const QString& path) {
        m_request.setMethod(HttpMethod::Put);
        m_request.setUrl(QUrl(path));
        return *this;
    }

    /**
     * @brief 设置 DELETE 请求
     * @param path 请求路径
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& del(const QString& path) {
        m_request.setMethod(HttpMethod::Delete);
        m_request.setUrl(QUrl(path));
        return *this;
    }

    /**
     * @brief 添加查询参数（数值类型）
     * @tparam T 参数值类型
     * @param key 参数名称
     * @param value 参数值
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    template<typename T>
    HttpApi& param(const QString& key, const T& value) {
        m_request.addParam(key, QString::number(value));
        return *this;
    }

    /**
     * @brief 添加查询参数（字符串类型）
     * @param key 参数名称
     * @param value 参数值
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& param(const QString& key, const QString& value) {
        m_request.addParam(key, value);
        return *this;
    }

    /**
     * @brief 添加请求头
     * @param key 请求头名称
     * @param value 请求头值
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& header(const QString& key, const QString& value) {
        m_request.addHeader(key, value);
        return *this;
    }

    /**
     * @brief 设置 JSON 请求体
     * @param json JSON 文档
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& jsonBody(const QJsonDocument& json) {
        m_request.setJsonBody(json);
        return *this;
    }

    /**
     * @brief 设置成功回调（泛型类型）
     * @tparam T 响应数据类型
     * @param callback 成功回调函数
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    template<typename T>
    HttpApi& onSuccess(std::function<void(const T&)> callback) {
        m_successCallback = [callback](const Result<HttpResponse>& result) {
            if (result.isOk()) {
                T data = parseResponse<T>(result.unwrap());
                callback(data);
            }
        };
        return *this;
    }

    /**
     * @brief 设置成功回调（JSON 文档）
     * @param callback 成功回调函数
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& onSuccess(std::function<void(const QJsonDocument&)> callback) {
        m_jsonCallback = callback;
        return *this;
    }

    /**
     * @brief 设置失败回调
     * @param callback 失败回调函数
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& onFailure(std::function<void(const Error&)> callback) {
        m_failureCallback = callback;
        return *this;
    }

    /**
     * @brief 执行异步请求
     */
    void execute() {
        m_client->sendAsync(m_request, [this](const Result<HttpResponse>& result) {
            if (result.isOk()) {
                const HttpResponse& response = result.unwrap();
                if (response.isSuccess()) {
                    if (m_successCallback) {
                        m_successCallback(result);
                    } else if (m_jsonCallback) {
                        m_jsonCallback(response.json());
                    }
                } else {
                    if (m_failureCallback) {
                        m_failureCallback(Error(ErrorCode::NetworkError, response.errorString()));
                    }
                }
            } else {
                if (m_failureCallback) {
                    m_failureCallback(result.unwrapErr());
                }
            }
        });
    }

    /**
     * @brief 执行同步请求
     * @return 包含 JSON 响应的 Result
     */
    Result<QJsonDocument> executeSync() {
        auto result = m_client->send(m_request);
        if (result.isOk()) {
            const HttpResponse& response = result.unwrap();
            if (response.isSuccess()) {
                return Result<QJsonDocument>(response.json());
            }
            return Result<QJsonDocument>(Error(ErrorCode::NetworkError, response.errorString()));
        }
        return Result<QJsonDocument>(result.unwrapErr());
    }

private:
    /**
     * @brief 解析响应数据（默认实现，子类可重写）
     * @tparam T 目标类型
     * @param response HTTP 响应
     * @return 解析后的数据
     */
    template<typename T>
    T parseResponse(const HttpResponse& response) {
        Q_UNUSED(response);
        return T();
    }

    std::shared_ptr<HttpClient> m_client;
    HttpRequest m_request;
    std::function<void(const QJsonDocument&)> m_jsonCallback;
    std::function<void(const Error&)> m_failureCallback;
    std::function<void(const Result<HttpResponse>&)> m_successCallback;
};

}

#endif
