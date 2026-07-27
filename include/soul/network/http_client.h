/**
 * @file http_client.h
 * @brief HTTP 客户端类
 * @details 封装 Qt 的 QNetworkAccessManager，提供同步/异步 HTTP 请求、拦截器支持和重试策略
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef SOUL_NETWORK_HTTP_CLIENT_H
#define SOUL_NETWORK_HTTP_CLIENT_H

#include "soul/network/network_global.h"
#include "soul/network/http_request.h"
#include "soul/network/http_response.h"
#include "soul/network/http/http_interceptor.h"
#include "soul/network/policy/retry_policy.h"
#include "soul/core/result.h"
#include <QNetworkAccessManager>
#include <memory>
#include <functional>
#include <vector>

namespace sc {
namespace network {

/**
 * @brief HTTP/2 配置(v1.8.0 新增)
 * @details 控制 HTTP/2 多路复用行为。
 *          Qt 6.5 通过 Http2AllowedAttribute 启用 HTTP/2,服务器不支持时自动降级。
 *          连接复用由 QNetworkAccessManager 内置连接池管理,无需显式配置。
 */
struct ConnectionPoolConfig {
    bool enableHttp2 = true;             ///< 是否启用 HTTP/2(与 setHttp2Enabled 同步)
    bool enableServerPush = false;       ///< 是否启用 HTTP/2 服务器推送(默认禁用,避免意外流量)
};

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
 * - HTTP/2 多路复用(v1.8.0 新增,默认启用)
 * - 连接池配置(v1.8.0 新增)
 *
 * 使用方式：
 * @code
 * HttpClient client;
 * client.setTimeout(30000);
 * client.setRetryPolicy(RetryPolicy(3));
 * // HTTP/2 默认启用,无需显式配置
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
 * @see HttpRequest, HttpResponse, IInterceptor, RetryPolicy, ConnectionPoolConfig
 */
class SC_NETWORK_EXPORT HttpClient : public QObject {
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
     * 
     * @warning 此方法会阻塞调用线程直到请求完成或超时。
     *          强烈建议在工作线程中调用，不要在 GUI 线程（主线程）中使用，
     *          否则会导致界面冻结。
     *          推荐使用 sendAsync() 方法进行异步请求。
     */
    Result<HttpResponse> send(const HttpRequest& request);

    /**
     * @brief 发送异步 HTTP 请求
     * @param request HTTP 请求
     * @param callback 响应回调函数
     */
    void sendAsync(const HttpRequest& request, ResponseCallback callback);

    /**
     * @brief 发送异步 HTTP 请求（带重试计数）
     * @param request HTTP 请求
     * @param callback 响应回调函数
     * @param retryCount 当前重试次数
     */
    void sendAsync(const HttpRequest& request, ResponseCallback callback, int retryCount);

    /**
     * @brief 添加请求拦截器
     * @param interceptor 拦截器
     */
    void addInterceptor(std::shared_ptr<HttpInterceptor> interceptor);

    /**
     * @brief 移除请求拦截器
     * @param interceptor 拦截器指针
     */
    void removeInterceptor(HttpInterceptor* interceptor);

    /**
     * @brief 设置重试策略
     * @param policy 重试策略
     */
    void setRetryPolicy(const RetryPolicy& policy);

    /**
     * @brief 获取重试策略
     * @return 当前重试策略
     */
    RetryPolicy retryPolicy() const;

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

    /**
     * @brief 启用或禁用 HTTP/2 多路复用(v1.8.0 新增)
     * @param enabled true 启用(默认),false 禁用
     * @details 默认启用 HTTP/2。服务器不支持时 Qt 自动降级到 HTTP/1.1。
     *          禁用场景:调试或与不支持 HTTP/2 的旧服务器兼容。
     */
    void setHttp2Enabled(bool enabled);

    /**
     * @brief 查询 HTTP/2 是否启用
     * @return true 启用,false 禁用
     */
    bool isHttp2Enabled() const;

    /**
     * @brief 设置 HTTP/2 配置(v1.8.0 新增)
     * @param config HTTP/2 配置(启用/禁用 HTTP/2、服务器推送)
     * @details 连接复用由 QNetworkAccessManager 内置连接池管理。
     *          enableHttp2 字段会同步到 setHttp2Enabled 状态。
     */
    void setConnectionPoolConfig(const ConnectionPoolConfig& config);

    /**
     * @brief 获取当前连接池配置
     * @return 连接池配置
     */
    ConnectionPoolConfig connectionPoolConfig() const;

private:
    /**
     * @brief 配置 SSL 设置
     */
    void setupSslConfiguration();

    /**
     * @brief 将 HTTP/2 与连接池配置应用到 QNetworkRequest
     * @param qrequest 待配置的 QNetworkRequest
     */
    void applyHttp2Config(QNetworkRequest& qrequest) const;

    QNetworkAccessManager* m_manager = nullptr;
    std::vector<std::shared_ptr<HttpInterceptor>> m_interceptors;
    RetryPolicy m_retryPolicy;
    int m_timeout = 30000;
    ConnectionPoolConfig m_poolConfig;  ///< HTTP/2 与连接池配置(默认启用 HTTP/2)
};

} // namespace network
} // namespace sc

#endif
