// ============================================================================
// cs_ipc_router.cpp — 进程间通信路由实现 [v2.5.0]
// ============================================================================

#include "soul/cs/cs_ipc_router.h"

#include <QLocalServer>
#include <QLocalSocket>
#include <QSharedMemory>
#include <QJsonDocument>
#include <QDataStream>
#include <QIODevice>
#include <QDebug>

namespace sc::cs {

// ============================================================================
// NamedPipeTransport 实现
// ============================================================================

NamedPipeTransport::NamedPipeTransport(QObject* parent)
    : QObject(parent)
{
}

NamedPipeTransport::~NamedPipeTransport() {
    stopServer();
    disconnectFromServer();
}

Result<void> NamedPipeTransport::startServer(const QString& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_server) {
        return Result<void>::err(Error(ErrorCode::AlreadyExists,
            QString("Server already running: %1").arg(name)));
    }

    m_serverName = name;
    m_server = new QLocalServer(this);

    QObject::connect(m_server, &QLocalServer::newConnection, this, &NamedPipeTransport::onNewConnection);

    if (!m_server->listen(name)) {
        QString error = m_server->errorString();
        delete m_server;
        m_server = nullptr;
        return Result<void>::err(Error(ErrorCode::InternalError, error));
    }

    return Result<void>::ok();
}

void NamedPipeTransport::stopServer() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }

    m_buffers.clear();
}

bool NamedPipeTransport::isServerRunning() const {
    return m_server && m_server->isListening();
}

Result<void> NamedPipeTransport::connectToServer(const QString& name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_client) {
        disconnectFromServer();
    }

    m_client = new QLocalSocket(this);
    QObject::connect(m_client, &QLocalSocket::readyRead, this, &NamedPipeTransport::onReadyRead);
    QObject::connect(m_client, &QLocalSocket::disconnected, this, &NamedPipeTransport::onDisconnected);
    QObject::connect(m_client, &QLocalSocket::errorOccurred, this, &NamedPipeTransport::onError);

    m_client->connectToServer(name);

    if (!m_client->waitForConnected(3000)) {
        QString error = m_client->errorString();
        delete m_client;
        m_client = nullptr;
        return Result<void>::err(Error(ErrorCode::ConnectionRefused, error));
    }

    return Result<void>::ok();
}

void NamedPipeTransport::disconnectFromServer() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_client) {
        m_client->disconnectFromServer();
        delete m_client;
        m_client = nullptr;
    }
}

bool NamedPipeTransport::isConnected() const {
    return m_client && m_client->state() == QLocalSocket::ConnectedState;
}

Result<void> NamedPipeTransport::sendRequest(const IpcRequest& request) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || m_client->state() != QLocalSocket::ConnectedState) {
        return Result<void>::err(Error(ErrorCode::NotConnected, "Not connected to server"));
    }

    QByteArray data = serializeRequest(request);
    m_client->write(data);
    m_client->flush();

    return Result<void>::ok();
}

Result<IpcResponse> NamedPipeTransport::sendRequestSync(const IpcRequest& request, int timeoutMs) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_client || m_client->state() != QLocalSocket::ConnectedState) {
        return Result<IpcResponse>::err(Error(ErrorCode::NotConnected, "Not connected to server"));
    }

    QByteArray data = serializeRequest(request);
    m_client->write(data);
    m_client->flush();

    if (!m_client->waitForReadyRead(timeoutMs)) {
        return Result<IpcResponse>::err(Error(ErrorCode::Timeout, "Timeout waiting for response"));
    }

    QByteArray responseData = m_client->readAll();
    return Result<IpcResponse>(deserializeResponse(responseData));
}

void NamedPipeTransport::setRequestHandler(RequestHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handler = std::move(handler);
}

// ============================================================================
// NamedPipeTransport 私有槽
// ============================================================================

void NamedPipeTransport::onNewConnection() {
    while (m_server->hasPendingConnections()) {
        QLocalSocket* socket = m_server->nextPendingConnection();
        QObject::connect(socket, &QLocalSocket::readyRead, this, &NamedPipeTransport::onReadyRead);
        QObject::connect(socket, &QLocalSocket::disconnected, this, &NamedPipeTransport::onDisconnected);
        QObject::connect(socket, &QLocalSocket::errorOccurred, this, &NamedPipeTransport::onError);
        m_buffers.insert(socket, QByteArray());
    }
}

void NamedPipeTransport::onReadyRead() {
    auto* socket = qobject_cast<QLocalSocket*>(sender());
    if (!socket) return;

    m_buffers[socket].append(socket->readAll());

    IpcRequest request = deserializeRequest(m_buffers[socket]);
    if (!request.requestId.isEmpty() && m_handler) {
        IpcResponse response = m_handler(request);
        socket->write(serializeResponse(response));
        socket->flush();
        m_buffers[socket].clear();
    }
}

void NamedPipeTransport::onDisconnected() {
    auto* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        m_buffers.remove(socket);
        socket->deleteLater();
    }
}

void NamedPipeTransport::onError(QLocalSocket::LocalSocketError error) {
    Q_UNUSED(error);
    auto* socket = qobject_cast<QLocalSocket*>(sender());
    if (socket) {
        qWarning() << "NamedPipeTransport: socket error:" << socket->errorString();
    }
}

// ============================================================================
// NamedPipeTransport 序列化/反序列化
// ============================================================================

QByteArray NamedPipeTransport::serializeRequest(const IpcRequest& request) const {
    QJsonObject obj;
    obj["method"]    = request.method;
    obj["path"]      = request.path;
    obj["headers"]   = request.headers;
    obj["params"]    = request.params;
    obj["body"]      = QString::fromUtf8(request.body.toBase64());
    obj["requestId"] = request.requestId;
    obj["timestamp"] = request.timestamp;
    obj["sourcePid"] = static_cast<qint64>(request.sourcePid);

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << static_cast<quint32>(data.size());
    frame.append(data);

    return frame;
}

IpcRequest NamedPipeTransport::deserializeRequest(const QByteArray& data) const {
    IpcRequest request;
    if (data.size() < 4) return request;

    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_5_15);
    quint32 size = 0;
    stream >> size;

    if (data.size() < static_cast<int>(size) + 4) return request;

    QByteArray jsonData = data.mid(4, size);
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) return request;

    QJsonObject obj = doc.object();
    request.method    = obj["method"].toString();
    request.path      = obj["path"].toString();
    request.headers   = obj["headers"].toObject();
    request.params    = obj["params"].toObject();
    request.body      = QByteArray::fromBase64(obj["body"].toString().toUtf8());
    request.requestId = obj["requestId"].toString();
    request.timestamp = static_cast<qint64>(obj["timestamp"].toDouble());
    request.sourcePid = static_cast<quint32>(obj["sourcePid"].toInt());

    return request;
}

QByteArray NamedPipeTransport::serializeResponse(const IpcResponse& response) const {
    QJsonObject obj;
    obj["statusCode"] = response.statusCode;
    obj["headers"]    = response.headers;
    obj["body"]       = QString::fromUtf8(response.body.toBase64());
    obj["requestId"]  = response.requestId;

    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    QByteArray frame;
    QDataStream stream(&frame, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_5_15);
    stream << static_cast<quint32>(data.size());
    frame.append(data);

    return frame;
}

IpcResponse NamedPipeTransport::deserializeResponse(const QByteArray& data) const {
    IpcResponse response;
    if (data.size() < 4) return response;

    QDataStream stream(data);
    stream.setVersion(QDataStream::Qt_5_15);
    quint32 size = 0;
    stream >> size;

    if (data.size() < static_cast<int>(size) + 4) return response;

    QByteArray jsonData = data.mid(4, size);
    QJsonDocument doc = QJsonDocument::fromJson(jsonData);
    if (!doc.isObject()) return response;

    QJsonObject obj = doc.object();
    response.statusCode = obj["statusCode"].toInt(200);
    response.headers    = obj["headers"].toObject();
    response.body       = QByteArray::fromBase64(obj["body"].toString().toUtf8());
    response.requestId  = obj["requestId"].toString();

    return response;
}

// ============================================================================
// SharedMemoryTransport 实现
// ============================================================================

SharedMemoryTransport::SharedMemoryTransport(const QString& key, int size)
    : m_key(key)
    , m_size(size)
    , m_sharedMemory(std::make_unique<QSharedMemory>(key))
{
}

SharedMemoryTransport::~SharedMemoryTransport() {
    detach();
}

Result<void> SharedMemoryTransport::create() {
    if (m_sharedMemory->isAttached()) {
        detach();
    }

    if (!m_sharedMemory->create(m_size)) {
        return Result<void>::err(Error(ErrorCode::InternalError,
            QString("Failed to create shared memory: %1").arg(m_sharedMemory->errorString())));
    }

    return Result<void>::ok();
}

Result<void> SharedMemoryTransport::attach() {
    if (m_sharedMemory->isAttached()) {
        return Result<void>::ok();
    }

    if (!m_sharedMemory->attach()) {
        return Result<void>::err(Error(ErrorCode::InternalError,
            QString("Failed to attach shared memory: %1").arg(m_sharedMemory->errorString())));
    }

    return Result<void>::ok();
}

void SharedMemoryTransport::detach() {
    if (m_sharedMemory->isAttached()) {
        m_sharedMemory->detach();
    }
}

Result<void> SharedMemoryTransport::write(const QByteArray& data) {
    if (!m_sharedMemory->isAttached()) {
        return Result<void>::err(Error(ErrorCode::NotConnected, "Shared memory not attached"));
    }

    m_sharedMemory->lock();

    int writeSize = qMin(data.size(), m_sharedMemory->size());
    char* dest = static_cast<char*>(m_sharedMemory->data());
    memcpy(dest, data.constData(), writeSize);

    m_sharedMemory->unlock();

    return Result<void>::ok();
}

Result<QByteArray> SharedMemoryTransport::read(int maxSize) {
    if (!m_sharedMemory->isAttached()) {
        return Result<QByteArray>::err(Error(ErrorCode::NotConnected, "Shared memory not attached"));
    }

    m_sharedMemory->lock();

    int readSize = m_sharedMemory->size();
    if (maxSize > 0) {
        readSize = qMin(readSize, maxSize);
    }

    QByteArray data(readSize, Qt::Uninitialized);
    const char* src = static_cast<const char*>(m_sharedMemory->constData());
    memcpy(data.data(), src, readSize);

    m_sharedMemory->unlock();

    return Result<QByteArray>(data);
}

bool SharedMemoryTransport::isAttached() const {
    return m_sharedMemory->isAttached();
}

int SharedMemoryTransport::size() const {
    return m_sharedMemory->size();
}

// ============================================================================
// CsIpcRouter 实现
// ============================================================================

CsIpcRouter::CsIpcRouter()
    : QObject(nullptr)
    , m_sharedMemory(std::make_unique<SharedMemoryTransport>("SoulCoreKit_SharedMem"))
{
    // 默认使用 NamedPipe 传输
    m_transport = std::make_unique<NamedPipeTransport>(this);
    m_transport->setRequestHandler([this](const IpcRequest& request) {
        return handleRequest(request);
    });
}

CsIpcRouter::~CsIpcRouter() {
    stopServer();
}

CsIpcRouter& CsIpcRouter::instance() {
    static CsIpcRouter s_instance;
    return s_instance;
}

// ============================================================================
// 服务端
// ============================================================================

Result<void> CsIpcRouter::startServer(const QString& serverName) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_serverRunning) {
        return Result<void>::err(Error(ErrorCode::AlreadyExists,
            QString("Server already running: %1").arg(serverName)));
    }

    auto result = m_transport->startServer(serverName);
    if (result.isOk()) {
        m_serverRunning = true;
    }
    return result;
}

void CsIpcRouter::stopServer() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_transport) {
        m_transport->stopServer();
    }
    m_serverRunning = false;
}

bool CsIpcRouter::isServerRunning() const {
    return m_serverRunning && m_transport && m_transport->isServerRunning();
}

// ============================================================================
// 客户端
// ============================================================================

Result<void> CsIpcRouter::connectToServer(const QString& serverName) {
    return m_transport->connectToServer(serverName);
}

void CsIpcRouter::disconnectFromServer() {
    if (m_transport) {
        m_transport->disconnectFromServer();
    }
}

Result<IpcResponse> CsIpcRouter::sendRequest(const IpcRequest& request, int timeoutMs) {
    return m_transport->sendRequestSync(request, timeoutMs);
}

// ============================================================================
// 路由注册
// ============================================================================

void CsIpcRouter::route(const QString& method, const QString& path, IpcHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routes.insert(makeRouteKey(method, path), std::move(handler));
}

void CsIpcRouter::removeRoute(const QString& method, const QString& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_routes.remove(makeRouteKey(method, path));
}

// ============================================================================
// 传输层
// ============================================================================

void CsIpcRouter::setTransport(std::unique_ptr<IpcTransport> transport) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_transport = std::move(transport);
    m_transport->setRequestHandler([this](const IpcRequest& request) {
        return handleRequest(request);
    });
}

// ============================================================================
// 请求处理
// ============================================================================

IpcResponse CsIpcRouter::handleRequest(const IpcRequest& request) {
    emit requestReceived(request.method, request.path);

    IpcResponse response;
    response.requestId = request.requestId;

    std::lock_guard<std::mutex> lock(m_mutex);
    QString key = makeRouteKey(request.method, request.path);
    auto it = m_routes.find(key);
    if (it != m_routes.end()) {
        response = it.value()(request);
        response.requestId = request.requestId;
    } else {
        response.statusCode = 404;
        response.body = QByteArray(R"({"error":"Route not found"})");
    }

    emit responseReady(response);
    return response;
}

QString CsIpcRouter::makeRouteKey(const QString& method, const QString& path) const {
    return method.toUpper() + ":" + path;
}

} // namespace sc::cs
