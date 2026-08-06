#ifndef SOUL_RPC_GRPC_SERVER_H
#define SOUL_RPC_GRPC_SERVER_H

// ============================================================================
// grpc_server.h / grpc_client.h — gRPC Server/Client [v2.5.0]
// ============================================================================
// gRPC 风格的 RPC 框架，基于 HTTP/2 + Protobuf 兼容层。
// 由于不引入 gRPC C++ 原生库（避免依赖膨胀），使用 HTTP/2 + JSON 作为传输层，
// 提供与 gRPC 兼容的 Service 定义和调用模型。
//
// 支持:
//   - Service 定义与注册
//   - Unary RPC (一元调用)
//   - Server Streaming RPC (服务端流式)
//   - Client Streaming RPC (客户端流式)
//   - Bidirectional Streaming RPC (双向流式)
//   - 拦截器链 (Interceptor)
//   - 超时与重试
//   - 元数据传递 (Metadata)
// ============================================================================

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "soul/core/result.h"
#include "soul/core/interface.h"

namespace sc {
namespace rpc {

// ============================================================================
// GrpcMetadata — gRPC 元数据 (对标 HTTP/2 headers)
// ============================================================================
struct GrpcMetadata {
    std::unordered_map<std::string, std::string> entries;

    void set(const std::string& key, const std::string& value);
    std::string get(const std::string& key, const std::string& defaultValue = "") const;
    bool has(const std::string& key) const;
    void remove(const std::string& key);
};

// ============================================================================
// GrpcContext — gRPC 调用上下文
// ============================================================================
struct GrpcContext {
    GrpcMetadata requestMetadata;
    GrpcMetadata responseMetadata;
    std::string peer;          // 对端地址
    std::string authority;     // :authority header
    int timeoutMs = 30000;     // 超时
    bool cancelled = false;    // 是否已取消
};

// ============================================================================
// GrpcStatus — gRPC 状态码
// ============================================================================
enum class GrpcStatusCode {
    OK = 0,
    CANCELLED = 1,
    UNKNOWN = 2,
    INVALID_ARGUMENT = 3,
    DEADLINE_EXCEEDED = 4,
    NOT_FOUND = 5,
    ALREADY_EXISTS = 6,
    PERMISSION_DENIED = 7,
    UNAUTHENTICATED = 16,
    RESOURCE_EXHAUSTED = 8,
    FAILED_PRECONDITION = 9,
    ABORTED = 10,
    OUT_OF_RANGE = 11,
    UNIMPLEMENTED = 12,
    INTERNAL = 13,
    UNAVAILABLE = 14,
    DATA_LOSS = 15
};

struct GrpcStatus {
    GrpcStatusCode code = GrpcStatusCode::OK;
    std::string message;
    QJsonObject details;
};

// ============================================================================
// GrpcService — gRPC 服务定义
// ============================================================================
class GrpcService : public IInterface {
public:
    ~GrpcService() override = default;

    virtual std::string serviceName() const = 0;
    virtual std::string interfaceName() const override { return serviceName(); }
};

// ============================================================================
// GrpcServer — gRPC 服务端
// ============================================================================
class GrpcServer : public QObject {
    Q_OBJECT
public:
    static GrpcServer& instance();

    // === 生命周期 ===
    Result<void> start(const QString& host = "0.0.0.0", int port = 50051);
    void stop();
    bool isRunning() const;

    // === 服务注册 ===
    Result<void> registerService(std::shared_ptr<GrpcService> service);
    Result<void> unregisterService(const std::string& serviceName);

    // === 拦截器 ===
    using GrpcInterceptor = std::function<Result<void>(GrpcContext& ctx)>;
    void addInterceptor(GrpcInterceptor interceptor);
    void removeInterceptors();

    // === 配置 ===
    void setMaxMessageSize(int bytes);
    void setKeepAliveTime(int ms);

signals:
    void started(int port);
    void stopped();
    void requestReceived(const QString& service, const QString& method);

private:
    GrpcServer() = default;
    ~GrpcServer() = default;

    std::unordered_map<std::string, std::shared_ptr<GrpcService>> m_services;
    std::vector<GrpcInterceptor> m_interceptors;
    mutable std::mutex m_mutex;
    bool m_running = false;
    int m_port = 0;
    int m_maxMessageSize = 4 * 1024 * 1024; // 4MB
};

// ============================================================================
// GrpcClient — gRPC 客户端
// ============================================================================
class GrpcClient {
public:
    explicit GrpcClient(const QString& target);
    ~GrpcClient();

    // === Unary RPC ===
    Result<QJsonObject> unaryCall(const std::string& service,
                                   const std::string& method,
                                   const QJsonObject& request,
                                   GrpcContext* ctx = nullptr,
                                   int timeoutMs = 30000);

    // === Server Streaming RPC ===
    using StreamCallback = std::function<void(const QJsonObject& response)>;
    using ErrorCallback = std::function<void(const GrpcStatus& status)>;

    Result<void> serverStreamingCall(const std::string& service,
                                      const std::string& method,
                                      const QJsonObject& request,
                                      StreamCallback onMessage,
                                      ErrorCallback onError = nullptr,
                                      int timeoutMs = 30000);

    // === Client Streaming RPC ===
    class ClientStreamWriter {
    public:
        void write(const QJsonObject& message);
        void writesDone();
        void cancel();
        bool isDone() const;

    private:
        friend class GrpcClient;
        // 内部实现
        bool m_done = false;
    };

    Result<std::shared_ptr<ClientStreamWriter>> clientStreamingCall(
        const std::string& service,
        const std::string& method,
        StreamCallback onResponse,
        ErrorCallback onError = nullptr);

    // === Bidirectional Streaming RPC ===
    class BidiStream {
    public:
        void write(const QJsonObject& message);
        void writesDone();
        void cancel();
        bool isDone() const;

    private:
        friend class GrpcClient;
        bool m_done = false;
    };

    Result<std::shared_ptr<BidiStream>> bidiStreamingCall(
        const std::string& service,
        const std::string& method,
        StreamCallback onMessage,
        ErrorCallback onError = nullptr);

    // === 连接管理 ===
    Result<void> connect();
    void disconnect();
    bool isConnected() const;

    // === 配置 ===
    void setDefaultTimeout(int ms);
    void setMaxRetries(int retries);
    void setEnableCompression(bool enable);

private:
    QString m_target;
    bool m_connected = false;
    int m_defaultTimeout = 30000;
    int m_maxRetries = 3;
    bool m_enableCompression = true;
    mutable std::mutex m_mutex;
};

} // namespace rpc
} // namespace sc

#endif // SOUL_RPC_GRPC_SERVER_H