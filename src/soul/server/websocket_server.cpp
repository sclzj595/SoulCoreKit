// ============================================================================
// websocket_server.cpp — WebSocket Server 实现(RFC 6455)
// ============================================================================

#include "soul/server/websocket_server.h"
#include "soul/logging/log_macros.h"

#include <QCryptographicHash>
#include <algorithm>

namespace sc {
namespace server {

// ============================================================================
// WebSocketSession 实现
// ============================================================================

WebSocketSession::WebSocketSession(QTcpSocket* socket, QObject* parent)
    : QObject(parent)
    , m_socket(socket) {
    if (m_socket) {
        m_socket->setParent(this);
        connect(m_socket, &QTcpSocket::readyRead, this, &WebSocketSession::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &WebSocketSession::onDisconnected);
    }
}

WebSocketSession::~WebSocketSession() {
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_closing.store(true);
        m_socket->disconnectFromHost();
    }
}

void WebSocketSession::sendText(const QString& message) {
    send(message.toUtf8(), true);
}

void WebSocketSession::sendBinary(const QByteArray& data) {
    send(data, false);
}

void WebSocketSession::send(const QByteArray& data, bool isText) {
    if (!m_open || m_closing.load()) {
        return;
    }
    sendFrame(isText ? WebSocketOpCode::Text : WebSocketOpCode::Binary, data);
}

void WebSocketSession::close(uint16_t code, const QString& reason) {
    if (!m_open || m_closing.exchange(true)) {
        return;  // 已在关闭中,防止递归
    }
    sendCloseFrame(code, reason);
    m_open = false;
    if (m_socket) {
        m_socket->flush();
        m_socket->disconnectFromHost();
    }
}

QHostAddress WebSocketSession::peerAddress() const {
    return m_socket ? m_socket->peerAddress() : QHostAddress();
}

bool WebSocketSession::isOpen() const noexcept {
    return m_open && m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void WebSocketSession::setProperty(const std::string& key, const QByteArray& value) {
    std::lock_guard<std::mutex> lock(m_propMutex);
    m_properties[key] = value;
}

QByteArray WebSocketSession::property(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_propMutex);
    auto it = m_properties.find(key);
    return it != m_properties.end() ? it->second : QByteArray();
}

// ============================================================================
// WebSocket 帧发送(RFC 6455 Section 5)
// ============================================================================

void WebSocketSession::sendFrame(WebSocketOpCode opCode, const QByteArray& payload) {
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    QByteArray frame;
    frame.reserve(2 + payload.size());

    // FIN + OpCode
    frame.append(static_cast<char>(0x80 | static_cast<uint8_t>(opCode)));

    // Mask + Payload length
    // Server → Client 不需要 mask(RFC 6455 Section 5.1)
    if (payload.size() <= 125) {
        frame.append(static_cast<char>(payload.size()));
    } else if (payload.size() <= 65535) {
        frame.append(static_cast<char>(126));
        uint16_t len = static_cast<uint16_t>(payload.size());
        frame.append(static_cast<char>((len >> 8) & 0xFF));
        frame.append(static_cast<char>(len & 0xFF));
    } else {
        frame.append(static_cast<char>(127));
        uint64_t len = static_cast<uint64_t>(payload.size());
        for (int i = 7; i >= 0; --i) {
            frame.append(static_cast<char>((len >> (i * 8)) & 0xFF));
        }
    }

    // Payload
    frame.append(payload);

    m_socket->write(frame);
}

void WebSocketSession::sendCloseFrame(uint16_t code, const QString& reason) {
    QByteArray payload;
    payload.append(static_cast<char>((code >> 8) & 0xFF));
    payload.append(static_cast<char>(code & 0xFF));
    if (!reason.isEmpty()) {
        payload.append(reason.toUtf8());
    }
    sendFrame(WebSocketOpCode::Close, payload);
}

// ============================================================================
// WebSocket 帧解析(RFC 6455 Section 5)
// ============================================================================

void WebSocketSession::onReadyRead() {
    if (!m_socket) {
        return;
    }

    m_buffer.append(m_socket->readAll());

    // 循环解析所有完整帧
    while (m_open && parseFrame()) {
        // 继续解析下一个帧
    }
}

void WebSocketSession::onDisconnected() {
    m_open = false;
    if (m_onClose) {
        m_onClose(this);
    }
    // Schedule deletion to trigger QObject::destroyed → removeSession
    deleteLater();
}

bool WebSocketSession::parseFrame() {
    if (m_buffer.size() < 2) {
        return false;  // 至少需要 2 字节头部
    }

    const uint8_t* data = reinterpret_cast<const uint8_t*>(m_buffer.constData());

    // 解析 FIN + OpCode
    uint8_t opCode = data[0] & 0x0F;
    bool masked = (data[1] & 0x80) != 0;

    // 解析 Payload Length
    uint64_t payloadLen = data[1] & 0x7F;
    size_t headerSize = 2;

    if (payloadLen == 126) {
        if (m_buffer.size() < 4) return false;
        payloadLen = (static_cast<uint16_t>(data[2]) << 8) | data[3];
        headerSize = 4;
    } else if (payloadLen == 127) {
        if (m_buffer.size() < 10) return false;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | data[2 + i];
        }
        headerSize = 10;
    }

    // Masking Key(4 bytes,Client → Server 必须)
    size_t maskOffset = headerSize;
    if (masked) {
        headerSize += 4;
    }

    // 检查完整帧是否到达
    if (static_cast<uint64_t>(m_buffer.size()) < headerSize + payloadLen) {
        return false;
    }

    // 提取 Payload
    QByteArray payload = m_buffer.mid(static_cast<int>(headerSize), static_cast<int>(payloadLen));

    // 去掩码(RFC 6455 Section 5.3)
    if (masked) {
        const uint8_t* mask = data + maskOffset;
        for (int i = 0; i < payload.size(); ++i) {
            payload[i] = payload[i] ^ mask[i % 4];
        }
    }

    // 移除已解析数据
    m_buffer.remove(0, static_cast<int>(headerSize + payloadLen));

    // 处理帧
    switch (static_cast<WebSocketOpCode>(opCode)) {
        case WebSocketOpCode::Text:
            if (m_onMessage) {
                m_onMessage(this, payload, true);
            }
            break;

        case WebSocketOpCode::Binary:
            if (m_onMessage) {
                m_onMessage(this, payload, false);
            }
            break;

        case WebSocketOpCode::Close:
            m_open = false;
            if (m_closing.exchange(true)) {
                // 我们已发起关闭,收到对方确认 Close 帧
            } else {
                // 客户端发起关闭,回复 Close 帧
                if (payload.size() >= 2) {
                    uint16_t code = (static_cast<uint16_t>(static_cast<uint8_t>(payload[0])) << 8)
                                  | static_cast<uint8_t>(payload[1]);
                    sendCloseFrame(code, QString());
                } else {
                    sendCloseFrame(1000, QString());
                }
            }
            if (m_socket) {
                m_socket->disconnectFromHost();
            }
            break;

        case WebSocketOpCode::Ping:
            // 自动回复 Pong(RFC 6455 Section 5.5.2)
            sendFrame(WebSocketOpCode::Pong, payload);
            break;

        case WebSocketOpCode::Pong:
            // Pong 帧,忽略(心跳由上层 QTimer 管理)
            break;

        case WebSocketOpCode::Continuation:
            // 不支持分片消息,忽略
            break;

        default:
            if (m_onError) {
                m_onError(this, "Unknown WebSocket opcode: " + std::to_string(opCode));
            }
            break;
    }

    return true;
}

// ============================================================================
// WebSocketServer 实现
// ============================================================================

WebSocketServer::WebSocketServer(QObject* parent)
    : QObject(parent) {
    m_tcpServer = new QTcpServer(this);
    connect(m_tcpServer, &QTcpServer::newConnection, this, &WebSocketServer::onNewConnection);
}

WebSocketServer::~WebSocketServer() {
    close();
}

bool WebSocketServer::listen(const QHostAddress& address, quint16 port) {
    if (!m_tcpServer->listen(address, port)) {
        SC_ERROR("WebSocketServer: failed to listen on " +
                 address.toString().toStdString() + ":" +
                 std::to_string(port) + " - " +
                 m_tcpServer->errorString().toStdString());
        return false;
    }
    SC_INFO("WebSocketServer: listening on " + address.toString().toStdString() +
            ":" + std::to_string(m_tcpServer->serverPort()));
    return true;
}

void WebSocketServer::close() {
    if (m_tcpServer) {
        m_tcpServer->close();
    }
    // 关闭所有活跃会话
    std::unordered_set<WebSocketSession*> sessionsCopy;
    {
        std::lock_guard<std::mutex> lock(m_sessionMutex);
        sessionsCopy = m_sessions;
        m_sessions.clear();
    }
    for (auto* session : sessionsCopy) {
        session->close(1001, "Server shutting down");
    }
}

bool WebSocketServer::isListening() const noexcept {
    return m_tcpServer && m_tcpServer->isListening();
}

quint16 WebSocketServer::serverPort() const noexcept {
    return m_tcpServer ? m_tcpServer->serverPort() : 0;
}

void WebSocketServer::setOnOpen(WebSocketSession::OnOpenCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onOpen = std::move(callback);
}

void WebSocketServer::setOnMessage(WebSocketSession::OnMessageCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onMessage = std::move(callback);
}

void WebSocketServer::setOnClose(WebSocketSession::OnCloseCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onClose = std::move(callback);
}

void WebSocketServer::setOnError(WebSocketSession::OnErrorCallback callback) {
    std::lock_guard<std::mutex> lock(m_callbackMutex);
    m_onError = std::move(callback);
}

void WebSocketServer::broadcast(const QByteArray& message, bool isText) {
    std::lock_guard<std::mutex> lock(m_sessionMutex);
    for (auto* session : m_sessions) {
        if (session->isOpen()) {
            session->send(message, isText);
        }
    }
}

size_t WebSocketServer::activeSessionCount() const {
    std::lock_guard<std::mutex> lock(m_sessionMutex);
    return m_sessions.size();
}

void WebSocketServer::onNewConnection() {
    while (m_tcpServer->hasPendingConnections()) {
        QTcpSocket* socket = m_tcpServer->nextPendingConnection();
        if (!socket) {
            continue;
        }

        // 等待 HTTP Upgrade 请求数据到达
        // 使用单次 readyRead 信号处理
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
            // 断开 lambda 连接,避免重复触发
            disconnect(socket, &QTcpSocket::readyRead, nullptr, nullptr);

            QByteArray request = socket->readAll();
            if (!performUpgrade(socket, request)) {
                socket->close();
                socket->deleteLater();
                return;
            }

            // 创建 WebSocketSession
            auto* session = new WebSocketSession(socket, this);

            // 复制回调到会话
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                session->setOnMessage(m_onMessage);
                session->setOnClose(m_onClose);
                session->setOnError(m_onError);
            }

            // 注册会话
            {
                std::lock_guard<std::mutex> lock(m_sessionMutex);
                m_sessions.insert(session);
            }

            // 会话关闭时自动移除
            connect(session, &QObject::destroyed, this, [this, session]() {
                removeSession(session);
            });

            emit sessionOpened(session);

            // 调用 onOpen 回调
            WebSocketSession::OnOpenCallback onOpen;
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                onOpen = m_onOpen;
            }
            if (onOpen) {
                onOpen(session);
            }
        });
    }
}

void WebSocketServer::removeSession(WebSocketSession* session) {
    std::lock_guard<std::mutex> lock(m_sessionMutex);
    m_sessions.erase(session);
    emit sessionClosed(session);
}

// ============================================================================
// HTTP Upgrade 握手(RFC 6455 Section 4)
// ============================================================================

bool WebSocketServer::performUpgrade(QTcpSocket* socket, const QByteArray& request) {
    // 解析 HTTP Upgrade 请求
    QString reqStr = QString::fromUtf8(request);

    // 检查是否为 GET 请求
    if (!reqStr.startsWith("GET ")) {
        SC_WARN("WebSocketServer: not a GET request");
        return false;
    }

    // 提取 Headers
    QString upgradeHeader;
    QString connectionHeader;
    QString keyHeader;
    QString versionHeader;

    QStringList lines = reqStr.split("\r\n");
    for (const QString& line : lines) {
        if (line.startsWith("Upgrade:", Qt::CaseInsensitive)) {
            upgradeHeader = line.mid(8).trimmed();
        } else if (line.startsWith("Connection:", Qt::CaseInsensitive)) {
            connectionHeader = line.mid(11).trimmed();
        } else if (line.startsWith("Sec-WebSocket-Key:", Qt::CaseInsensitive)) {
            keyHeader = line.mid(18).trimmed();
        } else if (line.startsWith("Sec-WebSocket-Version:", Qt::CaseInsensitive)) {
            versionHeader = line.mid(22).trimmed();
        }
    }

    // 验证 Upgrade 头
    if (upgradeHeader.compare("websocket", Qt::CaseInsensitive) != 0) {
        SC_WARN("WebSocketServer: missing or invalid Upgrade header");
        socket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
        return false;
    }

    // 验证 Connection 头(需包含 Upgrade)
    if (!connectionHeader.contains("Upgrade", Qt::CaseInsensitive)) {
        SC_WARN("WebSocketServer: missing Connection: Upgrade header");
        socket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
        return false;
    }

    // 验证 Sec-WebSocket-Key
    if (keyHeader.isEmpty()) {
        SC_WARN("WebSocketServer: missing Sec-WebSocket-Key header");
        socket->write("HTTP/1.1 400 Bad Request\r\n\r\n");
        return false;
    }

    // 验证 Sec-WebSocket-Version(必须为 13)
    if (versionHeader != "13") {
        SC_WARN("WebSocketServer: unsupported WebSocket version: " + versionHeader.toStdString());
        QByteArray response = "HTTP/1.1 426 Upgrade Required\r\n"
                              "Sec-WebSocket-Version: 13\r\n"
                              "\r\n";
        socket->write(response);
        return false;
    }

    // 计算 Accept Key
    QByteArray acceptKey = computeAcceptKey(keyHeader.toUtf8());

    // 发送 101 Switching Protocols
    QByteArray response = "HTTP/1.1 101 Switching Protocols\r\n"
                          "Upgrade: websocket\r\n"
                          "Connection: Upgrade\r\n"
                          "Sec-WebSocket-Accept: " + acceptKey + "\r\n"
                          "\r\n";
    socket->write(response);
    socket->flush();

    return true;
}

QByteArray WebSocketServer::computeAcceptKey(const QByteArray& clientKey) {
    // RFC 6455 Section 4.2.2:
    // Sec-WebSocket-Accept = base64(sha1(Sec-WebSocket-Key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"))
    QByteArray magic = clientKey + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    QByteArray hash = QCryptographicHash::hash(magic, QCryptographicHash::Sha1);
    return hash.toBase64();
}

} // namespace server
} // namespace sc