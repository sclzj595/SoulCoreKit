/**
 * @file core/inetwork.h
 * @brief 网络接口抽象
 * @details 定义网络通信的统一接口，支持 HTTP/TCP/WebSocket 等协议
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CORE_INETWORK_H
#define SOUL_NETWORK_CORE_INETWORK_H

#include <memory>
#include <functional>
#include <QUrl>
#include "soul/core/interface.h"
#include "soul/core/result.h"
#include "soul/network/core/network_message.h"
#include "soul/network/core/network_state.h"
#include "soul/network/policy/inetwork_policy.h"
#include "soul/network/interceptor/i_interceptor.h"

namespace sc {
namespace network {

/// @brief 网络通信统一抽象接口
///
/// 定义所有网络协议（HTTP/TCP/WebSocket/MQTT）的统一操作契约。
/// 支持同步/异步发送、状态查询、拦截器链和策略注入。
///
/// @par 设计模式
/// - **策略模式**: setPolicy() 注入重试/熔断/超时策略
/// - **责任链模式**: addInterceptor() 构建拦截器链
///
/// @see HttpClientAdapter, TcpClientAdapter, WsClientAdapter
class INetwork : public IInterface {
public:
    using ResponseCallback = std::function<void(const Result<NetworkMessage>&)>;

    ~INetwork() override = default;

    /// @brief 连接到指定 URL
    /// @param url 目标地址（协议+主机+端口+路径）
    virtual void connectTo(const QUrl& url) = 0;

    /// @brief 断开当前连接
    virtual void disconnect() = 0;

    /// @brief 检查连接状态
    /// @return true 表示已连接
    virtual bool isConnected() const = 0;

    /// @brief 同步发送消息
    /// @param message 网络消息（含 payload 和 headers）
    /// @return 响应消息或错误
    virtual Result<NetworkMessage> send(const NetworkMessage& message) = 0;

    /// @brief 异步发送消息
    /// @param message 网络消息
    /// @param callback 响应回调（在调用线程执行）
    virtual void sendAsync(const NetworkMessage& message, ResponseCallback callback) = 0;

    /// @brief 获取当前网络状态
    /// @return NetworkState 枚举值
    virtual NetworkState state() const = 0;

    /// @brief 注入网络策略（重试/熔断/超时）
    /// @param policy 策略实例
    virtual void setPolicy(std::shared_ptr<INetworkPolicy> policy) = 0;

    /// @brief 添加拦截器到责任链
    /// @param interceptor 拦截器实例
    virtual void addInterceptor(std::shared_ptr<IInterceptor<NetworkMessage, NetworkMessage>> interceptor) = 0;

    std::string interfaceName() const override {
        return "sc::network::INetwork";
    }
};

} // namespace network
} // namespace sc

#endif
