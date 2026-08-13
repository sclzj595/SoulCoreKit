#include "soul/rpc/grpc_server.h"

namespace sc {
namespace rpc {

// ============================================================================
// GrpcMetadata
// ============================================================================
void GrpcMetadata::set(const std::string& key, const std::string& value) {
    entries[key] = value;
}

std::string GrpcMetadata::get(const std::string& key, const std::string& defaultValue) const {
    auto it = entries.find(key);
    if (it != entries.end()) {
        return it->second;
    }
    return defaultValue;
}

bool GrpcMetadata::has(const std::string& key) const {
    return entries.find(key) != entries.end();
}

void GrpcMetadata::remove(const std::string& key) {
    entries.erase(key);
}

// ============================================================================
// GrpcServer
// ============================================================================
GrpcServer& GrpcServer::instance() {
    static GrpcServer s_instance;
    return s_instance;
}

Result<void> GrpcServer::start(const QString& host, int port) {
    Q_UNUSED(host);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_port = port;
    m_running = true;
    emit started(port);
    return Result<void>::ok();
}

void GrpcServer::stop() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_running = false;
    emit stopped();
}

bool GrpcServer::isRunning() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_running;
}

Result<void> GrpcServer::registerService(std::shared_ptr<GrpcService> service) {
    if (!service) {
        return Result<void>::err(Error(ErrorCode::InvalidArgument, "Null service"));
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    m_services[service->serviceName()] = service;
    return Result<void>::ok();
}

Result<void> GrpcServer::unregisterService(const std::string& serviceName) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_services.find(serviceName);
    if (it == m_services.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
            QString("Service not found: %1").arg(QString::fromStdString(serviceName))));
    }
    m_services.erase(it);
    return Result<void>::ok();
}

void GrpcServer::addInterceptor(GrpcInterceptor interceptor) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_interceptors.push_back(std::move(interceptor));
}

void GrpcServer::removeInterceptors() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_interceptors.clear();
}

void GrpcServer::setMaxMessageSize(int bytes) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_maxMessageSize = bytes;
}

void GrpcServer::setKeepAliveTime(int ms) {
    Q_UNUSED(ms);
}

// ============================================================================
// GrpcClient::ClientStreamWriter
// ============================================================================
void GrpcClient::ClientStreamWriter::write(const QJsonObject& message) {
    Q_UNUSED(message);
}

void GrpcClient::ClientStreamWriter::writesDone() {
    m_done = true;
}

void GrpcClient::ClientStreamWriter::cancel() {
    m_done = true;
}

bool GrpcClient::ClientStreamWriter::isDone() const {
    return m_done;
}

// ============================================================================
// GrpcClient::BidiStream
// ============================================================================
void GrpcClient::BidiStream::write(const QJsonObject& message) {
    Q_UNUSED(message);
}

void GrpcClient::BidiStream::writesDone() {
    m_done = true;
}

void GrpcClient::BidiStream::cancel() {
    m_done = true;
}

bool GrpcClient::BidiStream::isDone() const {
    return m_done;
}

// ============================================================================
// GrpcClient
// ============================================================================
GrpcClient::GrpcClient(const QString& target)
    : m_target(target) {
}

GrpcClient::~GrpcClient() {
    disconnect();
}

Result<QJsonObject> GrpcClient::unaryCall(const std::string& service,
                                           const std::string& method,
                                           const QJsonObject& request,
                                           GrpcContext* ctx,
                                           int timeoutMs) {
    Q_UNUSED(service);
    Q_UNUSED(method);
    Q_UNUSED(request);
    Q_UNUSED(ctx);
    Q_UNUSED(timeoutMs);
    return Result<QJsonObject>(QJsonObject());
}

Result<void> GrpcClient::serverStreamingCall(const std::string& service,
                                              const std::string& method,
                                              const QJsonObject& request,
                                              StreamCallback onMessage,
                                              ErrorCallback onError,
                                              int timeoutMs) {
    Q_UNUSED(service);
    Q_UNUSED(method);
    Q_UNUSED(request);
    Q_UNUSED(onMessage);
    Q_UNUSED(onError);
    Q_UNUSED(timeoutMs);
    return Result<void>::ok();
}

Result<std::shared_ptr<GrpcClient::ClientStreamWriter>> GrpcClient::clientStreamingCall(
    const std::string& service,
    const std::string& method,
    StreamCallback onResponse,
    ErrorCallback onError) {
    Q_UNUSED(service);
    Q_UNUSED(method);
    Q_UNUSED(onResponse);
    Q_UNUSED(onError);
    return Result<std::shared_ptr<ClientStreamWriter>>(std::make_shared<ClientStreamWriter>());
}

Result<std::shared_ptr<GrpcClient::BidiStream>> GrpcClient::bidiStreamingCall(
    const std::string& service,
    const std::string& method,
    StreamCallback onMessage,
    ErrorCallback onError) {
    Q_UNUSED(service);
    Q_UNUSED(method);
    Q_UNUSED(onMessage);
    Q_UNUSED(onError);
    return Result<std::shared_ptr<BidiStream>>(std::make_shared<BidiStream>());
}

Result<void> GrpcClient::connect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = true;
    return Result<void>::ok();
}

void GrpcClient::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connected = false;
}

bool GrpcClient::isConnected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connected;
}

void GrpcClient::setDefaultTimeout(int ms) {
    m_defaultTimeout = ms;
}

void GrpcClient::setMaxRetries(int retries) {
    m_maxRetries = retries;
}

void GrpcClient::setEnableCompression(bool enable) {
    m_enableCompression = enable;
}

} // namespace rpc
} // namespace sc
