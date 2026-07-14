#ifndef SOUL_NETWORK_TCP_CLIENT_H
#define SOUL_NETWORK_TCP_CLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <functional>

namespace sc {

/**
 * @class TcpClient
 * @brief TCP 客户端封装类，支持自动重连
 *
 * TcpClient 封装了 Qt 的 QTcpSocket，提供：
 * - 数据发送和接收
 * - 连接/断开回调
 * - 自动重连机制
 *
 * 使用方式：
 * @code
 * TcpClient client;
 * client.setDataCallback([](const QByteArray& data) {
 *     // 处理数据
 * });
 * client.connectToHost("localhost", 8080);
 * @endcode
 *
 * @see WebSocket
 */
class TcpClient : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 数据接收回调类型
     */
    using DataCallback = std::function<void(const QByteArray& data)>;

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
    explicit TcpClient(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~TcpClient();

    /**
     * @brief 连接到 TCP 服务器
     * @param host 服务器地址
     * @param port 服务器端口
     */
    void connectToHost(const QString& host, quint16 port);

    /**
     * @brief 断开与服务器的连接
     */
    void disconnectFromHost();

    /**
     * @brief 发送数据
     * @param data 要发送的数据
     */
    void send(const QByteArray& data);

    /**
     * @brief 检查是否已连接
     * @return 如果已连接返回 true
     */
    bool isConnected() const;

    /**
     * @brief 设置数据接收回调
     * @param callback 回调函数
     */
    void setDataCallback(DataCallback callback);

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

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    QTcpSocket* m_socket = nullptr;
    QString m_host;
    quint16 m_port = 0;

    DataCallback m_dataCallback;
    ConnectedCallback m_connectedCallback;
    DisconnectedCallback m_disconnectedCallback;

    bool m_autoReconnect = false;
};

}

#endif
