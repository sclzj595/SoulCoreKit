#ifndef SOUL_NETWORK_HTTP_CLIENT_H
#define SOUL_NETWORK_HTTP_CLIENT_H

#include "soul/network/http_request.h"
#include "soul/network/http_response.h"
#include "soul/network/i_interceptor.h"
#include "soul/network/policy/retry_policy.h"
#include "soul/core/result.h"
#include <QNetworkAccessManager>
#include <memory>
#include <functional>
#include <vector>

namespace sc {

/**
 * @class HttpClient
 * @brief HTTP 客户端类，支持同步/异步请求和拦截器
 *
 * HttpClient 封装了 Qt 的 QNetworkAccessManager，提供：
 * - 同步和异步 HTTP 请求
 * - 请求拦截器支持
 * - 可配置的重试策略
 * - 超时设置
 * - SSL 配置
 *
 * 使用方式：
 * @code
 * HttpClient client;
 * client.setTimeout(30000);
 * client.setRetryPolicy(RetryPolicy(3));
 *
 * // 同步请求
 * auto result = client.send(HttpRequest(HttpMethod::Get, QUrl("https://api.example.com")));
 * if (result.isOk()) {
 *     auto response = result.unwrap();
 * }
 *
 * // 异步请求
 * client.sendAsync(request, [](const Result<HttpResponse>& result) {
 *     // 处理响应
 * });
 * @endcode
 *
 * @see HttpRequest, HttpResponse, IInterceptor, RetryPolicy
 */
class HttpClient : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 响应回调函数类型
     */
    using ResponseCallback = std::function<void(const Result<HttpResponse>&)>;

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit HttpClient(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~HttpClient();

    /**
     * @brief 发送同步 HTTP 请求
     * @param request HTTP 请求
     * @return 包含响应的 Result
     */
    Result<HttpResponse> send(const HttpRequest& request);

    /**
     * @brief 发送异步 HTTP 请求
     * @param request HTTP 请求
     * @param callback 响应回调函数
     */
    void sendAsync(const HttpRequest& request, ResponseCallback callback);

    /**
     * @brief 添加请求拦截器
     * @param interceptor 拦截器
     */
    void addInterceptor(std::shared_ptr<IInterceptor> interceptor);

    /**
     * @brief 移除请求拦截器
     * @param interceptor 拦截器指针
     */
    void removeInterceptor(IInterceptor* interceptor);

    /**
     * @brief 设置重试策略
     * @param policy 重试策略
     */
    void setRetryPolicy(const network::RetryPolicy& policy);

    /**
     * @brief 获取重试策略
     * @return 当前重试策略
     */
    network::RetryPolicy retryPolicy() const;

    /**
     * @brief 设置超时时间
     * @param ms 超时时间（毫秒）
     */
    void setTimeout(int ms);

    /**
     * @brief 获取超时时间
     * @return 超时时间（毫秒）
     */
    int timeout() const;

private:
    /**
     * @brief 配置 SSL 设置
     */
    void setupSslConfiguration();

    QNetworkAccessManager* m_manager = nullptr;
    std::vector<std::shared_ptr<IInterceptor>> m_interceptors;
    network::RetryPolicy m_retryPolicy;
    int m_timeout = 30000;
};

}

#endif
