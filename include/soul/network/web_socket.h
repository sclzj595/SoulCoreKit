#ifndef SOUL_NETWORK_WEB_SOCKET_H
#define SOUL_NETWORK_WEB_SOCKET_H

#include <QObject>
#include <QWebSocket>
#include <functional>
#include <memory>

namespace sc {

/**
 * @class WebSocket
 * @brief WebSocket 客户端封装类，支持自动重连和心跳
 *
 * WebSocket 封装了 Qt 的 QWebSocket，提供：
 * - 文本和二进制消息发送
 * - 连接/断开回调
 * - 自动重连机制
 * - 可配置的重连间隔
 *
 * 使用方式：
 * @code
 * WebSocket ws;
 * ws.setAutoReconnect(true);
 * ws.setMessageCallback([](const QString& msg) {
 *     // 处理消息
 * });
 * ws.connectToServer(QUrl("ws://localhost:8080"));
 * @endcode
 *
 * @see TcpClient
 */
class WebSocket : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 文本消息回调类型
     */
    using MessageCallback = std::function<void(const QString& message)>;

    /**
     * @brief 二进制消息回调类型
     */
    using BinaryCallback = std::function<void(const QByteArray& data)>;

    /**
     * @brief 连接成功回调类型
     */
    using ConnectedCallback = std::function<void()>;

    /**
     * @brief 断开连接回调类型
     */
    using DisconnectedCallback = std::function<void()>;

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit WebSocket(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~WebSocket();

    /**
     * @brief 连接到 WebSocket 服务器
     * @param url 服务器 URL
     */
    void connectToServer(const QUrl& url);

    /**
     * @brief 断开与服务器的连接
     */
    void disconnectFromServer();

    /**
     * @brief 发送文本消息
     * @param message 文本消息
     */
    void sendTextMessage(const QString& message);

    /**
     * @brief 发送二进制消息
     * @param data 二进制数据
     */
    void sendBinaryMessage(const QByteArray& data);

    /**
     * @brief 检查是否已连接
     * @return 如果已连接返回 true
     */
    bool isConnected() const;

    /**
     * @brief 设置文本消息回调
     * @param callback 回调函数
     */
    void setMessageCallback(MessageCallback callback);

    /**
     * @brief 设置二进制消息回调
     * @param callback 回调函数
     */
    void setBinaryCallback(BinaryCallback callback);

    /**
     * @brief 设置连接成功回调
     * @param callback 回调函数
     */
    void setConnectedCallback(ConnectedCallback callback);

    /**
     * @brief 设置断开连接回调
     * @param callback 回调函数
     */
    void setDisconnectedCallback(DisconnectedCallback callback);

    /**
     * @brief 设置自动重连
     * @param enabled 是否启用自动重连
     */
    void setAutoReconnect(bool enabled);

    /**
     * @brief 检查是否启用自动重连
     * @return 如果启用返回 true
     */
    bool autoReconnect() const;

    /**
     * @brief 设置重连间隔
     * @param ms 重连间隔（毫秒）
     */
    void setReconnectInterval(int ms);

    /**
     * @brief 获取重连间隔
     * @return 重连间隔（毫秒）
     */
    int reconnectInterval() const;

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onBinaryMessageReceived(const QByteArray& data);
    void onError(QAbstractSocket::SocketError error);
    void reconnect();

private:
    QWebSocket* m_socket = nullptr;
    QUrl m_url;

    MessageCallback m_messageCallback;
    BinaryCallback m_binaryCallback;
    ConnectedCallback m_connectedCallback;
    DisconnectedCallback m_disconnectedCallback;

    bool m_autoReconnect = false;
    int m_reconnectInterval = 5000;
};

}

#endif
