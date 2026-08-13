#ifndef SOUL_RPC_IRPC_TRANSPORT_H
#define SOUL_RPC_IRPC_TRANSPORT_H

// ============================================================================
// irpc_transport.h — RPC 传输抽象接口 [v2.7.0 增强]
// ============================================================================
//
// 设计原则:
//   - Transport 抽象: HTTP/WebSocket/TCP 皆可为 Transport
//   - Codec 抽象: JSON/Protobuf/MessagePack 皆可为 Codec
//   - gRPC 作为 Adapter: 核心不绑定 gRPC
//
// 架构:
//   SoulCore RPC
//        │
//   ┌────┼────┐
//   ↓    ↓    ↓
//  HTTP WS   TCP Transport
//   │    │    │
//   └────┼────┘
//        ↓
//      Codec
//        │
//   ┌────┼────┐
//   ↓    ↓    ↓
//  JSON Proto MsgPack
//        │
//   gRPC Adapter
//
// 用法:
//   // HTTP Transport
//   auto transport = RpcTransportFactory::createHttp("http://localhost:8080/rpc");
//
//   // WebSocket Transport
//   auto transport = RpcTransportFactory::createWebSocket("ws://localhost:8080/rpc");
//
//   // TCP Transport (CS 场景)
//   auto transport = RpcTransportFactory::createTcp("localhost", 9090);
//
//   // gRPC Adapter (P3, 不污染核心)
//   auto transport = RpcTransportFactory::createGrpc("localhost:50051");

#include <QString>
#include <functional>
#include <memory>
#include <string>
#include <variant>

#include "soul/core/result.h"
#include "soul/utils/json/json_helper.h"

namespace sc { namespace rpc {

// ============================================================================
// RpcRequest / RpcResponse — RPC 请求/响应模型
// ============================================================================

struct RpcRequest {
    QString serviceName;
    QString methodName;
    sc::json::Json params;
    QString requestId;
};

struct RpcResponse {
    bool success;
    sc::json::Json data;
    QString errorMessage;
    QString requestId;
};

// ============================================================================
// IRpcCodec — RPC 编解码器接口 [v2.7.0 新增]
// ============================================================================
// 负责 RpcRequest/RpcResponse 与传输层字节流的互转。
// 默认 JSON Codec，可扩展 Protobuf/MessagePack。

class IRpcCodec {
public:
    virtual ~IRpcCodec() = default;

    /// @brief 编码请求为字节流
    virtual QByteArray encodeRequest(const RpcRequest& request) = 0;

    /// @brief 编码响应为字节流
    virtual QByteArray encodeResponse(const RpcResponse& response) = 0;

    /// @brief 解码字节流为请求
    virtual Result<RpcRequest> decodeRequest(const QByteArray& data) = 0;

    /// @brief 解码字节流为响应
    virtual Result<RpcResponse> decodeResponse(const QByteArray& data) = 0;

    /// @return Codec 名称
    virtual std::string name() const = 0;
};

// ============================================================================
// RpcTransportType — Transport 类型枚举 [v2.7.0 新增]
// ============================================================================

enum class RpcTransportType {
    Http,
    WebSocket,
    Tcp,
    Grpc   // Adapter
};

// ============================================================================
// IRpcTransport — RPC 传输接口
// ============================================================================

class IRpcTransport {
public:
    virtual ~IRpcTransport() = default;

    /// @brief 发送请求并等待响应
    virtual Result<RpcResponse> sendRequest(const RpcRequest& request) = 0;

    /// @brief 启动传输层
    virtual void start() = 0;

    /// @brief 停止传输层
    virtual void stop() = 0;

    /// @return 是否运行中
    virtual bool isRunning() const = 0;

    /// @return Transport 类型 [v2.7.0 新增]
    virtual RpcTransportType type() const = 0;

    /// @brief 设置 Codec [v2.7.0 新增]
    virtual void setCodec(std::shared_ptr<IRpcCodec> codec) = 0;
};

// ============================================================================
// RpcTransportFactory — Transport 工厂 [v2.7.0 新增]
// ============================================================================
// 根据类型创建对应的 Transport 实例。

class RpcTransportFactory {
public:
    /// @brief 创建 HTTP Transport
    static std::shared_ptr<IRpcTransport> createHttp(const std::string& baseUrl);

    /// @brief 创建 WebSocket Transport
    static std::shared_ptr<IRpcTransport> createWebSocket(const std::string& url);

    /// @brief 创建 TCP Transport (CS 场景)
    static std::shared_ptr<IRpcTransport> createTcp(const std::string& host, uint16_t port);

    /// @brief 创建 gRPC Transport (Adapter, P3)
    static std::shared_ptr<IRpcTransport> createGrpc(const std::string& target);
};

using RpcHandler = std::function<RpcResponse(const RpcRequest&)>;

}} // namespace sc::rpc

#endif // SOUL_RPC_IRPC_TRANSPORT_H
