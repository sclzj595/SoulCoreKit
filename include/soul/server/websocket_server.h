#ifndef SOUL_SERVER_WEBSOCKET_SERVER_H
#define SOUL_SERVER_WEBSOCKET_SERVER_H

// ============================================================================
// websocket_server.h — WebSocket Server(CS 架构 Server 端实时双向通信)
// ============================================================================
//
// 设计目标: 对标 SpringBoot WebSocket 支持,基于 QTcpServer + HTTP Upgrade
// 握手实现 RFC 6455 WebSocket 协议,提供 Server 端实时推送能力。
//
// 核心组件:
//   - WebSocketServer:  WebSocket 服务端,管理监听和会话
//   - WebSocketSession: 单个 WebSocket 连接,管理帧解析和回调
//   - WebSocketOpCode:  操作码枚举(RFC 6455 Section 5.2)
//
// 支持特性:
//   - RFC 6455 基础帧解析(Text/Binary/Close/Ping/Pong)
//   - HTTP Upgrade 握手(支持 Sec-WebSocket-Key/Sec-WebSocket-Accept)
//   - 文本和二进制消息
//   - onOpen/onMessage/onClose/onError 回调
//   - 广播消息(broadcast)
//   - 自动 Ping/Pong 心跳(可选)
//
// 不支持(保持轻量):
//   - WebSocket 扩展(permessage-deflate 等)
//   - 子协议协商(Sec-WebSocket-Protocol)
//   - wss:// (TLS,后续可扩展)
//
// 用法:
//   sc::server::WebSocketServer wsServer;
//   wsServer.setOnMessage([](WebSocketSession* session,
//                            const QByteArray& msg, bool isText) {
//       session->send("Echo: " + msg);
//   });
//   wsServer.listen(QHostAddress::Any, 8080);

#include <QObject>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QByteArray>
#include <QString>
#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace sc {
namespace server {

// ============================================================================
// WebSocketOpCode — WebSocket 操作码(RFC 6455 Section 5.2)
// ============================================================================
enum class WebSocketOpCode : uint8_t {
    Continuation = 0x0,
    Text         = 0x1,
    Binary       = 0x2,
    Close        = 0x8,
    Ping         = 0x9,
    Pong         = 0xA
};

// ============================================================================
// WebSocketSession — WebSocket 连接会话
// ============================================================================
//
// 管理单个 WebSocket 连接的生命周期,负责帧解析和消息回调。
// 实例由 WebSocketServer 创建和管理,用户不应直接创建。
//
// @thread_safety 非线程安全,应在 Qt 事件循环线程使用
class WebSocketSession : public QObject {
    Q_OBJECT
public:
    using OnOpenCallback    = std::function<void(WebSocketSession*)>;
    using OnMessageCallback = std::function<void(WebSocketSession*, const QByteArray&, bool isText)>;
    using OnCloseCallback   = std::function<void(WebSocketSession*)>;
    using OnErrorCallback   = std::function<void(WebSocketSession*, const std::string& error)>;

    /// @brief 构造 WebSocket 会话(由 WebSocketServer 内部调用)
    /// @param socket  已连接的 QTcpSocket(已完成 HTTP Upgrade)
    /// @param parent  Qt 父对象
    explicit WebSocketSession(QTcpSocket* socket, QObject* parent = nullptr);
    ~WebSocketSession() override;

    WebSocketSession(const WebSocketSession&) = delete;
    WebSocketSession& operator=(const WebSocketSession&) = delete;

    /// @brief 发送文本消息
    void sendText(const QString& message);

    /// @brief 发送二进制消息
    void sendBinary(const QByteArray& data);

    /// @brief 发送消息(自动选择文本/二进制)
    /// @param data   消息内容
    /// @param isText true=文本帧, false=二进制帧
    void send(const QByteArray& data, bool isText = true);

    /// @brief 关闭连接(发送 Close 帧)
    /// @param code   关闭状态码(RFC 6455 Section 7.4)
    /// @param reason 关闭原因
    void close(uint16_t code = 1000, const QString& reason = QString());

    /// @return 客户端地址
    QHostAddress peerAddress() const;

    /// @return 连接是否活跃
    bool isOpen() const noexcept;

    /// @brief 设置自定义属性(用于在回调间传递上下文)
    void setProperty(const std::string& key, const QByteArray& value);
    QByteArray property(const std::string& key) const;

    // 回调设置(通常由 WebSocketServer 统一设置)
    void setOnMessage(OnMessageCallback callback) { m_onMessage = std::move(callback); }
    void setOnClose(OnCloseCallback callback)     { m_onClose = std::move(callback); }
    void setOnError(OnErrorCallback callback)     { m_onError = std::move(callback); }

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    /// @brief 发送 WebSocket 帧
    void sendFrame(WebSocketOpCode opCode, const QByteArray& payload);

    /// @brief 发送 Close 帧
    void sendCloseFrame(uint16_t code, const QString& reason);

    /// @brief 解析 WebSocket 帧
    /// @return true 解析成功, false 协议错误
    bool parseFrame();

    QTcpSocket*      m_socket = nullptr;
    QByteArray        m_buffer;       ///< 接收缓冲
    bool              m_open = true;
    std::atomic<bool> m_closing{false}; ///< 正在关闭(防止递归关闭)

    OnMessageCallback m_onMessage;
    OnCloseCallback   m_onClose;
    OnErrorCallback   m_onError;

    // 自定义属性(线程安全)
    mutable std::mutex              m_propMutex;
    std::map<std::string, QByteArray> m_properties;
};

// ============================================================================
// WebSocketServer — WebSocket 服务端
// ============================================================================
//
// 基于 QTcpServer 的 WebSocket Server,处理 HTTP Upgrade 握手并管理会话。
//
// @thread_safety 回调注册线程安全,会话管理在 Qt 事件循环线程
class WebSocketServer : public QObject {
    Q_OBJECT
public:
    explicit WebSocketServer(QObject* parent = nullptr);
    ~WebSocketServer() override;

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    /// @brief 开始监听指定端口
    /// @param address 监听地址(默认 Any=0.0.0.0)
    /// @param port    监听端口(0=自动分配)
    /// @return 是否成功
    bool listen(const QHostAddress& address = QHostAddress::Any, quint16 port = 0);

    /// @brief 停止监听并关闭所有连接
    void close();

    /// @return 是否正在监听
    bool isListening() const noexcept;

    /// @return 实际监听端口
    quint16 serverPort() const noexcept;

    // === 回调设置(所有会话共享) ===

    /// @brief 设置连接建立回调
    void setOnOpen(WebSocketSession::OnOpenCallback callback);

    /// @brief 设置消息接收回调
    void setOnMessage(WebSocketSession::OnMessageCallback callback);

    /// @brief 设置连接关闭回调
    void setOnClose(WebSocketSession::OnCloseCallback callback);

    /// @brief 设置错误回调
    void setOnError(WebSocketSession::OnErrorCallback callback);

    // === 会话管理 ===

    /// @brief 向所有活跃会话广播消息
    void broadcast(const QByteArray& message, bool isText = true);

    /// @return 活跃会话数
    size_t activeSessionCount() const;

signals:
    /// @brief 新 WebSocket 连接建立
    void sessionOpened(WebSocketSession* session);

    /// @brief WebSocket 连接关闭
    void sessionClosed(WebSocketSession* session);

private slots:
    void onNewConnection();

private:
    /// @brief 处理 HTTP Upgrade 握手(RFC 6455 Section 4)
    bool performUpgrade(QTcpSocket* socket, const QByteArray& request);

    /// @brief 计算 Sec-WebSocket-Accept
    static QByteArray computeAcceptKey(const QByteArray& clientKey);

    /// @brief 移除会话
    void removeSession(WebSocketSession* session);

    QTcpServer* m_tcpServer = nullptr;

    // 回调
    mutable std::mutex                        m_callbackMutex;
    WebSocketSession::OnOpenCallback          m_onOpen;
    WebSocketSession::OnMessageCallback       m_onMessage;
    WebSocketSession::OnCloseCallback         m_onClose;
    WebSocketSession::OnErrorCallback         m_onError;

    // 活跃会话
    mutable std::mutex                        m_sessionMutex;
    std::unordered_set<WebSocketSession*>     m_sessions;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_WEBSOCKET_SERVER_H