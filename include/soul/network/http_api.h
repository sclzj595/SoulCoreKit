/**
 * @file http_api.h
 * @brief HTTP API 链式调用封装
 * @details 泛型模板类，提供类型安全的 HTTP API 调用封装，支持 GET/POST/PUT/DELETE 和 JSON 响应
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef SOUL_NETWORK_HTTP_API_H
#define SOUL_NETWORK_HTTP_API_H

#include "soul/network/http_client.h"
#include "soul/network/http_request.h"
#include "soul/network/http_response.h"
#include "soul/core/result.h"
#include "soul/utils/json/json_helper.h"
#include <functional>
#include <memory>

namespace sc {
namespace network {

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
 *    .onSuccess([](const sc::json::Json& data) {
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
    HttpApi& jsonBody(const sc::json::Json& json) {
        m_request.setJsonBody(json);
        return *this;
    }

    /**
     * @brief 设置成功回调（JSON 文档）
     * @param callback 成功回调函数
     * @return 当前 HttpApi 对象引用（链式调用）
     */
    HttpApi& onSuccess(std::function<void(const sc::json::Json&)> callback) {
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
                    if (m_jsonCallback) {
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
    Result<sc::json::Json> executeSync() {
        auto result = m_client->send(m_request);
        if (result.isOk()) {
            const HttpResponse& response = result.unwrap();
            if (response.isSuccess()) {
                return Result<sc::json::Json>(response.json());
            }
            return Result<sc::json::Json>(Error(ErrorCode::NetworkError, response.errorString()));
        }
        return Result<sc::json::Json>(result.unwrapErr());
    }

private:
    std::shared_ptr<HttpClient> m_client;
    HttpRequest m_request;
    std::function<void(const sc::json::Json&)> m_jsonCallback;
    std::function<void(const Error&)> m_failureCallback;
};

} // namespace network
} // namespace sc

#endif
