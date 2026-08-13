#ifndef SOUL_CS_IPC_ROUTER_H
#define SOUL_CS_IPC_ROUTER_H

// ============================================================================
// cs_ipc_router.h — 进程间通信路由 [v2.5.0]
// ============================================================================
// 提供 CS 架构的进程间通信路由能力，支持:
//   - NamedPipe (Windows) / Unix Domain Socket (Linux/macOS)
//   - QLocalServer/QLocalSocket 本地通信
//   - 共享内存 (QSharedMemory) 大数据传输
//   - 与 CsRouter 统一的路由匹配机制
// ============================================================================

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <QHash>
#include <QLocalSocket>
#include <functional>
#include <memory>
#include <mutex>

#include "soul/cs/cs_global.h"
#include "soul/core/result.h"

class QLocalServer;
class QLocalSocket;
class QSharedMemory;

namespace sc::cs {

// ============================================================================
// IpcRequest / IpcResponse
// ============================================================================
struct IpcRequest {
    QString method;         // 请求方法 (GET/POST/PUT/DELETE)
    QString path;           // 请求路径 (/user/list)
    QJsonObject headers;    // 请求头
    QJsonObject params;     // 路径/查询参数
    QByteArray body;        // 请求体
    QString requestId;      // 请求 ID
    qint64 timestamp = 0;   // 时间戳
    quint32 sourcePid = 0;  // 来源进程 PID
};

struct IpcResponse {
    int statusCode = 200;
    QJsonObject headers;
    QByteArray body;
    QString requestId;
};

// ============================================================================
// IpcTransport — IPC 传输抽象
// ============================================================================
class IpcTransport {
public:
    virtual ~IpcTransport() = default;

    virtual Result<void> startServer(const QString& name) = 0;
    virtual void stopServer() = 0;
    virtual bool isServerRunning() const = 0;

    virtual Result<void> connectToServer(const QString& name) = 0;
    virtual void disconnectFromServer() = 0;
    virtual bool isConnected() const = 0;

    virtual Result<void> sendRequest(const IpcRequest& request) = 0;
    virtual Result<IpcResponse> sendRequestSync(const IpcRequest& request, int timeoutMs = 5000) = 0;

    using RequestHandler = std::function<IpcResponse(const IpcRequest&)>;
    virtual void setRequestHandler(RequestHandler handler) = 0;
};

// ============================================================================
// NamedPipeTransport — QLocalServer/QLocalSocket 实现
// ============================================================================
class NamedPipeTransport : public QObject, public IpcTransport {
    Q_OBJECT
public:
    explicit NamedPipeTransport(QObject* parent = nullptr);
    ~NamedPipeTransport() override;

    Result<void> startServer(const QString& name) override;
    void stopServer() override;
    bool isServerRunning() const override;

    Result<void> connectToServer(const QString& name) override;
    void disconnectFromServer() override;
    bool isConnected() const override;

    Result<void> sendRequest(const IpcRequest& request) override;
    Result<IpcResponse> sendRequestSync(const IpcRequest& request, int timeoutMs = 5000) override;

    void setRequestHandler(RequestHandler handler) override;

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();
    void onError(QLocalSocket::LocalSocketError error);

private:
    QByteArray serializeRequest(const IpcRequest& request) const;
    IpcRequest deserializeRequest(const QByteArray& data) const;
    QByteArray serializeResponse(const IpcResponse& response) const;
    IpcResponse deserializeResponse(const QByteArray& data) const;

    QLocalServer* m_server = nullptr;
    QLocalSocket* m_client = nullptr;
    QHash<QLocalSocket*, QByteArray> m_buffers;
    RequestHandler m_handler;
    std::mutex m_mutex;
    QString m_serverName;
};

// ============================================================================
// SharedMemoryTransport — 共享内存大数据传输
// ============================================================================
class SharedMemoryTransport {
public:
    explicit SharedMemoryTransport(const QString& key, int size = 64 * 1024);
    ~SharedMemoryTransport();

    Result<void> create();
    Result<void> attach();
    void detach();

    Result<void> write(const QByteArray& data);
    Result<QByteArray> read(int maxSize = -1);

    bool isAttached() const;
    int size() const;

private:
    QString m_key;
    int m_size;
    std::unique_ptr<QSharedMemory> m_sharedMemory;
};

// ============================================================================
// CsIpcRouter — IPC 路由
// ============================================================================
class CsIpcRouter : public QObject {
    Q_OBJECT
public:
    static CsIpcRouter& instance();

    // === 服务端 ===
    Result<void> startServer(const QString& serverName = "SoulCoreKit");
    void stopServer();
    bool isServerRunning() const;

    // === 客户端 ===
    Result<void> connectToServer(const QString& serverName = "SoulCoreKit");
    void disconnectFromServer();
    Result<IpcResponse> sendRequest(const IpcRequest& request, int timeoutMs = 5000);

    // === 路由注册 ===
    using IpcHandler = std::function<IpcResponse(const IpcRequest&)>;
    void route(const QString& method, const QString& path, IpcHandler handler);
    void removeRoute(const QString& method, const QString& path);

    // === 传输层 ===
    void setTransport(std::unique_ptr<IpcTransport> transport);

    // === 共享内存 ===
    SharedMemoryTransport& sharedMemory() { return *m_sharedMemory; }

signals:
    void requestReceived(const QString& method, const QString& path);
    void responseReady(const IpcResponse& response);
    void serverError(const QString& error);

private:
    CsIpcRouter();
    ~CsIpcRouter() override;

    IpcResponse handleRequest(const IpcRequest& request);
    QString makeRouteKey(const QString& method, const QString& path) const;

    std::unique_ptr<IpcTransport> m_transport;
    std::unique_ptr<SharedMemoryTransport> m_sharedMemory;
    QHash<QString, IpcHandler> m_routes;
    std::mutex m_mutex;
    bool m_serverRunning = false;
};

} // namespace sc::cs

#endif // SOUL_CS_IPC_ROUTER_H