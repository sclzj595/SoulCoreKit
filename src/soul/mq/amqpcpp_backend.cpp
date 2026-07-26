#include "soul/mq/amqpcpp_backend.h"

#ifdef SOUL_ENABLE_RABBITMQ

#include "soul/logging/log_macros.h"
#include "soul/core/uuid.h"
#include <QHostAddress>
#include <QTimer>
#include <QEventLoop>
#include <chrono>
#include <future>

namespace sc {
namespace mq {

// ============================================================================
// AmqpCppBackend::QtTcpHandler 实现
// ============================================================================

AmqpCppBackend::QtTcpHandler::QtTcpHandler(QObject* parent)
    : m_parent(parent) {
}

AmqpCppBackend::QtTcpHandler::~QtTcpHandler() {
    // 清理所有 notifier
    m_notifiers.clear();
}

void AmqpCppBackend::QtTcpHandler::monitor(AMQP::TcpConnection* conn, int fd, int flags) {
    m_conn = conn;

    auto it = m_notifiers.find(fd);
    if (it == m_notifiers.end()) {
        // 创建新的 notifier entry
        NotifierEntry entry;
        entry.readNotifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Read, m_parent);
        entry.writeNotifier = std::make_unique<QSocketNotifier>(fd, QSocketNotifier::Write, m_parent);

        // 连接 activated 信号到 processConnection
        QObject::connect(entry.readNotifier.get(), &QSocketNotifier::activated,
                          [this, conn](int) { processConnection(conn); });
        QObject::connect(entry.writeNotifier.get(), &QSocketNotifier::activated,
                          [this, conn](int) { processConnection(conn); });

        m_notifiers[fd] = std::move(entry);
        it = m_notifiers.find(fd);
    }

    // 根据 flags 启用/禁用 notifier
    it->second.readNotifier->setEnabled(flags & AMQP::readable);
    it->second.writeNotifier->setEnabled(flags & AMQP::writable);
}

void AmqpCppBackend::QtTcpHandler::onConnected(AMQP::TcpConnection* conn) {
    (void)conn;
    SC_INFO("AmqpCppBackend: TCP connection established, AMQP handshake in progress");
    if (m_handshakeCb) {
        m_handshakeCb(true, "");
    }
}

void AmqpCppBackend::QtTcpHandler::onError(AMQP::TcpConnection* conn, const char* message) {
    (void)conn;
    SC_ERROR(std::string("AmqpCppBackend: connection error: ") + (message ? message : "unknown"));
    if (m_handshakeCb) {
        m_handshakeCb(false, message ? message : "unknown error");
    }
}

void AmqpCppBackend::QtTcpHandler::onClosed(AMQP::TcpConnection* conn) {
    (void)conn;
    SC_INFO("AmqpCppBackend: connection closed");
    // 清理所有 notifier
    m_notifiers.clear();
}

void AmqpCppBackend::QtTcpHandler::processConnection(AMQP::TcpConnection* conn) {
    // 处理 amqpcpp 的网络事件
    conn->process();
}

// ============================================================================
// AmqpCppBackend 实现
// ============================================================================

AmqpCppBackend::AmqpCppBackend() {
    // 心跳定时器必须在主线程创建(遵守线程亲和性)
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(30000);  // 30s 默认心跳
    QObject::connect(m_heartbeatTimer, &QTimer::timeout, [this]() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_connection && m_connected.load(std::memory_order_acquire)) {
            m_connection->heartbeat();
        }
    });
}

AmqpCppBackend::~AmqpCppBackend() {
    // 必须在主线程析构(遵守线程亲和性文档约束)
    // QTimer(父对象=this)会随本对象析构自动销毁
    disconnect();
}

Result<void> AmqpCppBackend::connect(const Config& config) {
    if (m_connected.load(std::memory_order_acquire)) {
        return Result<void>::err(Error(ErrorCode::InvalidArgument,
                                        "AmqpCppBackend: already connected"));
    }

    // 锁内:重置状态,创建 handler,设置握手回调
    std::unique_ptr<QtTcpHandler> handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_config = config;

        SC_INFO("AmqpCppBackend: connecting to " + config.host.toStdString() +
                ":" + std::to_string(config.port) +
                " vhost=" + config.virtualHost.toStdString());

        handler = std::make_unique<QtTcpHandler>(this);

        // 重置握手状态(使用成员变量,onConnected/onError 回调设置)
        m_handshakeDone.store(false, std::memory_order_release);
        m_handshakeSuccess.store(false, std::memory_order_release);
        m_handshakeError.clear();

        // 重置 channel 状态(使用成员变量,避免 onReady/onError 捕获局部变量 UAF)
        m_channelReady.store(false, std::memory_order_release);
        m_channelError.store(false, std::memory_order_release);
        m_channelErrorMsg.clear();

        handler->setHandshakeCallback([this](bool success, const std::string& error) {
            m_handshakeSuccess.store(success, std::memory_order_release);
            m_handshakeError = error;
            m_handshakeDone.store(true, std::memory_order_release);
        });
    }

    // 锁外:构造 address,创建 connection,等待握手
    std::string scheme = config.enableSsl ? "amqps://" : "amqp://";
    std::string address = scheme +
                          config.username.toStdString() + ":" +
                          config.password.toStdString() + "@" +
                          config.host.toStdString() + ":" +
                          std::to_string(config.port) +
                          config.virtualHost.toStdString();

    AMQP::Address amqpAddress(address);

    std::shared_ptr<AMQP::TcpConnection> connection;
    std::shared_ptr<AMQP::TcpChannel> channel;

    try {
        // 创建 TcpConnection(异步发起 TCP + AMQP 握手)
        // 注意:handler 所有权暂不转移,connection 通过裸指针引用 handler
        connection = std::make_shared<AMQP::TcpConnection>(handler.get(), amqpAddress);

        // 等待握手完成(锁外,QEventLoop 处理 Qt 事件)
        auto result = waitForHandshake(
            std::chrono::milliseconds(config.connectionTimeout > 0 ? config.connectionTimeout : 30000));
        if (!result.isOk()) {
            return result;
        }

        // 创建 channel
        channel = std::make_shared<AMQP::TcpChannel>(connection.get());

        // 注册 channel 级别持久回调(捕获 this,使用成员变量,避免 UAF)
        // onReady:通常只触发一次
        channel->onReady([this]() {
            m_channelReady.store(true, std::memory_order_release);
        });

        // onError:channel 生命周期内可能多次触发(网络断开等)
        // 必须使用成员变量存储错误信息,不能捕获局部变量
        channel->onError([this](const char* msg) {
            SC_ERROR(std::string("AmqpCppBackend: channel error: ") + (msg ? msg : "unknown"));
            std::lock_guard<std::mutex> lock(m_mutex);
            m_channelErrorMsg = msg ? msg : "unknown";
            m_channelError.store(true, std::memory_order_release);
        });

        // 等待 channel ready(锁外,使用 waitForDone)
        if (!waitForDone(m_channelReady, std::chrono::seconds(10))) {
            return Result<void>::err(Error(ErrorCode::Timeout,
                                            "AmqpCppBackend: channel open timed out"));
        }

        if (m_channelError.load(std::memory_order_acquire)) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return Result<void>::err(Error(ErrorCode::NetworkError,
                                            "AmqpCppBackend: channel error: " + m_channelErrorMsg));
        }

        // 锁内:提交最终状态
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_handler = std::move(handler);
            m_connection = connection;
            m_channel = channel;
            m_connected.store(true, std::memory_order_release);
        }

        // 配置心跳间隔
        m_heartbeatTimer->setInterval(
            std::chrono::seconds(config.heartbeatInterval > 0 ? config.heartbeatInterval : 60));
        startHeartbeat();

        SC_INFO("AmqpCppBackend: connection established successfully");
        return Result<void>::ok();

    } catch (const std::exception& e) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        std::string("AmqpCppBackend: connect exception: ") + e.what()));
    }
}

void AmqpCppBackend::disconnect() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_connected.load(std::memory_order_acquire)) {
        return;
    }

    stopHeartbeat();
    m_consuming.store(false, std::memory_order_release);

    // 关闭 channel
    if (m_channel) {
        try {
            m_channel->close();
        } catch (const std::exception& e) {
            SC_ERROR(std::string("AmqpCppBackend: channel close exception: ") + e.what());
        } catch (...) {
            SC_ERROR("AmqpCppBackend: channel close unknown exception");
        }
        m_channel.reset();
    }

    // 关闭 connection
    if (m_connection) {
        try {
            m_connection->close();
        } catch (const std::exception& e) {
            SC_ERROR(std::string("AmqpCppBackend: connection close exception: ") + e.what());
        } catch (...) {
            SC_ERROR("AmqpCppBackend: connection close unknown exception");
        }
        m_connection.reset();
    }

    m_handler.reset();
    m_consumers.clear();
    m_connected.store(false, std::memory_order_release);

    SC_INFO("AmqpCppBackend: disconnected");
}

bool AmqpCppBackend::isConnected() const {
    return m_connected.load(std::memory_order_acquire);
}

Result<void> AmqpCppBackend::declareExchange(const QString& name, ExchangeType type,
                                              bool durable, bool autoDelete) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }
    if (name.isEmpty()) {
        return Result<void>::err(Error(ErrorCode::InvalidArgument,
                                        "AmqpCppBackend: exchange name must not be empty"));
    }

    // 锁内:仅拷贝 channel shared_ptr(防止中途被 reset)
    std::shared_ptr<AMQP::TcpChannel> channel;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_channel) {
            return Result<void>::err(Error(ErrorCode::InternalError,
                                            "AmqpCppBackend: channel not available"));
        }
        channel = m_channel;
    }

    // 锁外:注册回调 + 等待(避免持有 m_mutex 进 QEventLoop 导致死锁)
    std::atomic<bool> done{false};
    std::atomic<bool> success{false};
    std::string errorMsg;

    AMQP::ExchangeType amqpType = toAmqpExchangeType(type);
    int flags = 0;
    if (durable) flags |= AMQP::durable;
    if (autoDelete) flags |= AMQP::autodelete;

    channel->declareExchange(name.toStdString(), amqpType, flags)
        .onSuccess([&done, &success]() {
            success.store(true, std::memory_order_release);
            done.store(true, std::memory_order_release);
        })
        .onError([&done, &success, &errorMsg](const char* msg) {
            errorMsg = msg ? msg : "unknown";
            success.store(false, std::memory_order_release);
            done.store(true, std::memory_order_release);
        });

    if (!waitForDone(done, std::chrono::seconds(10))) {
        return Result<void>::err(Error(ErrorCode::Timeout,
                                        "AmqpCppBackend::declareExchange timed out"));
    }
    if (!success.load(std::memory_order_acquire)) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        "AmqpCppBackend::declareExchange failed: " + errorMsg));
    }
    return Result<void>::ok();
}

Result<QString> AmqpCppBackend::declareQueue(const QString& name, bool durable,
                                              bool exclusive, bool autoDelete) {
    if (!isConnected()) {
        return Error(ErrorCode::NotConnected, "AmqpCppBackend: not connected");
    }

    // 锁内:仅拷贝 channel
    std::shared_ptr<AMQP::TcpChannel> channel;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_channel) {
            return Error(ErrorCode::InternalError, "AmqpCppBackend: channel not available");
        }
        channel = m_channel;
    }

    // 锁外:注册回调 + 等待
    std::atomic<bool> done{false};
    std::atomic<bool> success{false};
    std::string declaredName;
    std::string errorMsg;

    int flags = 0;
    if (durable) flags |= AMQP::durable;
    if (exclusive) flags |= AMQP::exclusive;
    if (autoDelete) flags |= AMQP::autodelete;

    std::string queueName = name.toStdString();
    channel->declareQueue(queueName, flags)
        .onSuccess([&done, &success, &declaredName, name](const std::string& declared) {
            declaredName = name.isEmpty() ? declared : name.toStdString();
            success.store(true, std::memory_order_release);
            done.store(true, std::memory_order_release);
        })
        .onError([&done, &success, &errorMsg](const char* msg) {
            errorMsg = msg ? msg : "unknown";
            success.store(false, std::memory_order_release);
            done.store(true, std::memory_order_release);
        });

    if (!waitForDone(done, std::chrono::seconds(10))) {
        return Error(ErrorCode::Timeout, "AmqpCppBackend::declareQueue timed out");
    }
    if (!success.load(std::memory_order_acquire)) {
        return Error(ErrorCode::NetworkError,
                      "AmqpCppBackend::declareQueue failed: " + errorMsg);
    }
    return Result<QString>::ok(QString::fromStdString(declaredName));
}

Result<void> AmqpCppBackend::bindQueue(const QString& queue, const QString& exchange,
                                        const QString& routingKey) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }

    // 锁内:仅拷贝 channel
    std::shared_ptr<AMQP::TcpChannel> channel;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_channel) {
            return Result<void>::err(Error(ErrorCode::InternalError,
                                            "AmqpCppBackend: channel not available"));
        }
        channel = m_channel;
    }

    // 锁外:注册回调 + 等待
    std::atomic<bool> done{false};
    std::atomic<bool> success{false};
    std::string errorMsg;

    channel->bindQueue(exchange.toStdString(),
                       queue.toStdString(),
                       routingKey.toStdString())
        .onSuccess([&done, &success]() {
            success.store(true, std::memory_order_release);
            done.store(true, std::memory_order_release);
        })
        .onError([&done, &success, &errorMsg](const char* msg) {
            errorMsg = msg ? msg : "unknown";
            success.store(false, std::memory_order_release);
            done.store(true, std::memory_order_release);
        });

    if (!waitForDone(done, std::chrono::seconds(10))) {
        return Result<void>::err(Error(ErrorCode::Timeout,
                                        "AmqpCppBackend::bindQueue timed out"));
    }
    if (!success.load(std::memory_order_acquire)) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        "AmqpCppBackend::bindQueue failed: " + errorMsg));
    }
    return Result<void>::ok();
}

Result<void> AmqpCppBackend::unbindQueue(const QString& queue, const QString& exchange,
                                          const QString& routingKey) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }

    // 锁内:仅拷贝 channel
    std::shared_ptr<AMQP::TcpChannel> channel;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_channel) {
            return Result<void>::err(Error(ErrorCode::InternalError,
                                            "AmqpCppBackend: channel not available"));
        }
        channel = m_channel;
    }

    // 锁外:注册回调 + 等待
    std::atomic<bool> done{false};
    std::atomic<bool> success{false};
    std::string errorMsg;

    channel->unbindQueue(exchange.toStdString(),
                         queue.toStdString(),
                         routingKey.toStdString())
        .onSuccess([&done, &success]() {
            success.store(true, std::memory_order_release);
            done.store(true, std::memory_order_release);
        })
        .onError([&done, &success, &errorMsg](const char* msg) {
            errorMsg = msg ? msg : "unknown";
            success.store(false, std::memory_order_release);
            done.store(true, std::memory_order_release);
        });

    if (!waitForDone(done, std::chrono::seconds(10))) {
        return Result<void>::err(Error(ErrorCode::Timeout,
                                        "AmqpCppBackend::unbindQueue timed out"));
    }
    if (!success.load(std::memory_order_acquire)) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        "AmqpCppBackend::unbindQueue failed: " + errorMsg));
    }
    return Result<void>::ok();
}

Result<void> AmqpCppBackend::publish(const AmqpMessage& message) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_channel) {
        return Result<void>::err(Error(ErrorCode::InternalError,
                                        "AmqpCppBackend: channel not available"));
    }

    try {
        AMQP::Message amqpMsg = toAmqpMessage(message);
        m_channel->publish(message.exchange.toStdString(),
                            message.routingKey.toStdString(),
                            amqpMsg,
                            message.deliveryMode == 2 ? AMQP::persistent : AMQP::nonpersistent);
        return Result<void>::ok();
    } catch (const std::exception& e) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        std::string("AmqpCppBackend::publish exception: ") + e.what()));
    }
}

Result<void> AmqpCppBackend::consume(const QString& queue, AmqpConsumeCallback callback,
                                      int prefetchCount) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }
    if (!callback) {
        return Result<void>::err(Error(ErrorCode::InvalidArgument,
                                        "AmqpCppBackend: consume callback must not be null"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_channel) {
        return Result<void>::err(Error(ErrorCode::InternalError,
                                        "AmqpCppBackend: channel not available"));
    }

    // 设置 QoS
    m_channel->setQos(prefetchCount > 0 ? prefetchCount : 1);

    std::string consumerTag = generateConsumerTag(queue);
    QString queueName = queue;

    // 注册消费者(回调通过 Qt 事件循环触发,无需等待)
    m_channel->consume(queue.toStdString(), consumerTag)
        .onReceived([this, callback, queueName](const AMQP::Message& msg,
                                                  uint64_t deliveryTag,
                                                  bool redelivered) {
            // 检查 consuming 标志,stopConsuming 后不再投递
            if (!m_consuming.load(std::memory_order_acquire)) {
                return;
            }
            AmqpDelivery delivery = fromAmqpMessage(
                msg, static_cast<qint64>(deliveryTag), redelivered,
                QString::fromStdString(msg.exchange()),
                QString::fromStdString(msg.routingkey()),
                queueName);
            callback(delivery);
        })
        .onError([](const char* msg) {
            SC_ERROR(std::string("AmqpCppBackend: consume error: ") + (msg ? msg : "unknown"));
        });

    ConsumerEntry entry;
    entry.callback = std::move(callback);
    entry.consumerTag = consumerTag;
    entry.prefetchCount = prefetchCount > 0 ? prefetchCount : 1;
    m_consumers[queue] = std::move(entry);

    SC_INFO("AmqpCppBackend: registered consumer for queue '" + queue.toStdString() +
            "' tag='" + consumerTag + "'");
    return Result<void>::ok();
}

Result<void> AmqpCppBackend::cancelConsume(const QString& queue) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }

    // 锁内:查找 consumerTag + 拷贝 channel(不立即 erase)
    std::shared_ptr<AMQP::TcpChannel> channel;
    std::string consumerTag;
    bool found = false;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_consumers.find(queue);
        if (it == m_consumers.end()) {
            // 幂等:不存在的订阅视为已取消
            return Result<void>::ok();
        }
        consumerTag = it->second.consumerTag;
        channel = m_channel;
        found = true;
    }

    // 锁外:调用 amqpcpp cancel(避免持有 m_mutex 进 QEventLoop 死锁)
    if (found && channel) {
        std::atomic<bool> done{false};
        std::atomic<bool> success{false};
        std::string errorMsg;

        channel->cancel(consumerTag)
            .onSuccess([&done, &success]() {
                success.store(true, std::memory_order_release);
                done.store(true, std::memory_order_release);
            })
            .onError([&done, &success, &errorMsg](const char* msg) {
                errorMsg = msg ? msg : "unknown";
                success.store(false, std::memory_order_release);
                done.store(true, std::memory_order_release);
            });

        if (!waitForDone(done, std::chrono::seconds(5))) {
            // 超时:不删除 m_consumers 记录,允许用户重试
            return Result<void>::err(Error(ErrorCode::Timeout,
                                            "AmqpCppBackend::cancelConsume timed out"));
        }
        if (!success.load(std::memory_order_acquire)) {
            // cancel 失败:不删除 m_consumers 记录,保持状态一致
            return Result<void>::err(Error(ErrorCode::NetworkError,
                                            "AmqpCppBackend::cancelConsume failed: " + errorMsg));
        }
    }

    // 锁内:cancel 成功后,仅当 consumerTag 仍是原来的才删除
    // 避免误删 cancel 期间通过 Qt 事件循环重入注册的新消费者(consume 可能覆盖 m_consumers[queue])
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_consumers.find(queue);
        if (it != m_consumers.end() && it->second.consumerTag == consumerTag) {
            m_consumers.erase(it);
        }
        // 若 consumerTag 不匹配,说明期间有新的 consume 注册,不删除新消费者记录
    }
    return Result<void>::ok();
}

Result<void> AmqpCppBackend::ack(qint64 deliveryTag, bool multiple) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_channel) {
        return Result<void>::err(Error(ErrorCode::InternalError,
                                        "AmqpCppBackend: channel not available"));
    }

    try {
        m_channel->ack(static_cast<uint64_t>(deliveryTag),
                        multiple ? AMQP::multiple : 0);
        return Result<void>::ok();
    } catch (const std::exception& e) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        std::string("AmqpCppBackend::ack exception: ") + e.what()));
    }
}

Result<void> AmqpCppBackend::nack(qint64 deliveryTag, bool requeue, bool multiple) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "AmqpCppBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_channel) {
        return Result<void>::err(Error(ErrorCode::InternalError,
                                        "AmqpCppBackend: channel not available"));
    }

    try {
        int flags = 0;
        if (requeue) flags |= AMQP::requeue;
        if (multiple) flags |= AMQP::multiple;
        m_channel->nack(static_cast<uint64_t>(deliveryTag), flags);
        return Result<void>::ok();
    } catch (const std::exception& e) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        std::string("AmqpCppBackend::nack exception: ") + e.what()));
    }
}

Result<void> AmqpCppBackend::reject(qint64 deliveryTag, bool requeue) {
    // reject 等价于 nack(requeue, multiple=false)
    return nack(deliveryTag, requeue, false);
}

void AmqpCppBackend::startConsuming() {
    // AmqpCppBackend 不需要独立分发线程,消息通过 Qt 事件循环回调
    m_consuming.store(true, std::memory_order_release);
    SC_INFO("AmqpCppBackend: consuming started (event-driven via Qt event loop)");
}

void AmqpCppBackend::stopConsuming() {
    m_consuming.store(false, std::memory_order_release);
    SC_INFO("AmqpCppBackend: consuming stopped");
}

// ============================================================================
// 私有辅助方法
// ============================================================================

AMQP::ExchangeType AmqpCppBackend::toAmqpExchangeType(ExchangeType type) {
    switch (type) {
    case ExchangeType::Direct:  return AMQP::direct;
    case ExchangeType::Fanout:  return AMQP::fanout;
    case ExchangeType::Topic:   return AMQP::topic;
    case ExchangeType::Headers: return AMQP::headers;
    }
    return AMQP::direct;
}

AMQP::Message AmqpCppBackend::toAmqpMessage(const AmqpMessage& message) {
    AMQP::Message amqpMsg(message.body.constData(),
                           static_cast<uint64_t>(message.body.size()));

    // 设置 envelope
    amqpMsg.setExchange(message.exchange.toStdString());
    amqpMsg.setRoutingKey(message.routingKey.toStdString());

    // 设置属性
    if (!message.messageId.isEmpty()) {
        amqpMsg.setMessageID(message.messageId.toStdString());
    }
    if (!message.correlationId.isEmpty()) {
        amqpMsg.setCorrelationID(message.correlationId.toStdString());
    }
    if (!message.replyTo.isEmpty()) {
        amqpMsg.setReplyTo(message.replyTo.toStdString());
    }
    if (!message.contentType.isEmpty()) {
        amqpMsg.setContentType(message.contentType.toStdString());
    }
    if (!message.contentEncoding.isEmpty()) {
        amqpMsg.setContentEncoding(message.contentEncoding.toStdString());
    }
    if (message.timestamp > 0) {
        amqpMsg.setTimestamp(static_cast<uint64_t>(message.timestamp));
    }
    amqpMsg.setDeliveryMode(message.deliveryMode == 2 ? AMQP::persistent : AMQP::nonpersistent);
    amqpMsg.setPriority(static_cast<uint8_t>(message.priority));

    return amqpMsg;
}

AmqpDelivery AmqpCppBackend::fromAmqpMessage(const AMQP::Message& msg,
                                               qint64 deliveryTag,
                                               bool redelivered,
                                               const QString& exchange,
                                               const QString& routingKey,
                                               const QString& queue) {
    AmqpDelivery delivery;
    delivery.deliveryTag = deliveryTag;
    delivery.redelivered = redelivered;
    delivery.exchange = exchange;
    delivery.routingKey = routingKey;
    delivery.queue = queue;

    AmqpMessage& message = delivery.message;
    message.exchange = exchange;
    message.routingKey = routingKey;
    message.body = QByteArray(msg.body(), static_cast<int>(msg.bodySize()));

    // 提取属性
    const auto& envelope = msg;
    message.messageId = QString::fromStdString(envelope.messageID());
    message.correlationId = QString::fromStdString(envelope.correlationID());
    message.replyTo = QString::fromStdString(envelope.replyTo());
    message.contentType = QString::fromStdString(envelope.contentType());
    message.contentEncoding = QString::fromStdString(envelope.contentEncoding());
    message.timestamp = static_cast<qint64>(envelope.timestamp());
    message.deliveryMode = envelope.deliveryMode() == AMQP::persistent ? 2 : 1;
    message.priority = envelope.priority();

    return delivery;
}

Result<void> AmqpCppBackend::waitForHandshake(std::chrono::milliseconds timeout) {
    // 使用 QEventLoop 嵌套处理 Qt 事件
    // 关键:QSocketNotifier::activated 信号需要 Qt 事件循环处理
    // 若用 std::condition_variable::wait 阻塞,信号无法触发,amqpcpp 无法 process(),握手永远不完成
    // 注意:调用者必须不持有 m_mutex,否则心跳定时器 lambda 会死锁
    if (!waitForDone(m_handshakeDone, timeout)) {
        return Result<void>::err(Error(ErrorCode::Timeout,
                                        "AmqpCppBackend: connect handshake timed out"));
    }

    if (!m_handshakeSuccess.load(std::memory_order_acquire)) {
        return Result<void>::err(Error(ErrorCode::NetworkError,
                                        "AmqpCppBackend: connect failed: " + m_handshakeError));
    }

    return Result<void>::ok();
}

bool AmqpCppBackend::waitForDone(const std::atomic<bool>& done,
                                    std::chrono::milliseconds timeout) {
    // 辅助方法:统一抽取异步操作的等待逻辑
    // 调用者必须不持有 m_mutex(详见头文件文档)
    QEventLoop loop;
    QTimer pollTimer;
    QTimer timeoutTimer;
    pollTimer.setInterval(50);  // 50ms 轮询
    timeoutTimer.setSingleShot(true);

    QObject::connect(&pollTimer, &QTimer::timeout, [&]() {
        if (done.load(std::memory_order_acquire)) {
            loop.quit();
        }
    });
    QObject::connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);

    pollTimer.start();
    timeoutTimer.start(timeout);
    loop.exec();
    pollTimer.stop();
    timeoutTimer.stop();

    return done.load(std::memory_order_acquire);
}

void AmqpCppBackend::startHeartbeat() {
    if (m_heartbeatTimer) {
        // 直接调用 start()(已在主线程,无需 invokeMethod)
        m_heartbeatTimer->start();
    }
}

void AmqpCppBackend::stopHeartbeat() {
    if (m_heartbeatTimer) {
        // 直接调用 stop()(已在主线程)
        m_heartbeatTimer->stop();
    }
}

std::string AmqpCppBackend::generateConsumerTag(const QString& queue) {
    return "sc.consumer." + queue.toStdString() + "." +
           Uuid::generate().substr(0, 8);
}

} // namespace mq
} // namespace sc

#endif // SOUL_ENABLE_RABBITMQ
