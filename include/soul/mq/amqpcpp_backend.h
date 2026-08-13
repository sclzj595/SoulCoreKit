#ifndef SOUL_MQ_AMQPCPP_BACKEND_H
#define SOUL_MQ_AMQPCPP_BACKEND_H

#include "soul/mq/iamqp_backend.h"

// AmqpCppBackend 仅在 ENABLE_RABBITMQ=ON 时编译
// 默认构建不引入 amqpcpp 依赖,保持零外部依赖
#ifdef SOUL_ENABLE_RABBITMQ

#include <QObject>
#include <QSocketNotifier>
#include <QTimer>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <future>

// amqpcpp 头文件
#include <amqpcpp.h>
#include <amqpcpp/linux_tcp.h>

namespace sc {
namespace mq {

/**
 * @class AmqpCppBackend
 * @brief 基于 amqpcpp 库的真实 AMQP 0.9.1 后端实现
 *
 * AmqpCppBackend 通过 [amqpcpp](https://github.com/CopernicaMarketingSoftware/AMQP-CPP)
 * 库提供真实的 RabbitMQ 通信能力,严格遵循 AMQP 0.9.1 协议规范。
 *
 * 设计要点:
 * - **Qt 事件循环集成**: 自定义 `QtTcpHandler` 通过 `QSocketNotifier` 将
 *   amqpcpp 的文件描述符事件集成到 Qt 主事件循环,避免独立线程
 * - **异步转同步**: amqpcpp 的所有操作通过回调通知结果,本后端通过
 *   `QEventLoop` 嵌套处理 Qt 事件,等待回调完成,避免主线程死锁
 * - **心跳保活**: amqpcpp 内置心跳,通过 `QTimer` 定期调用 `heartbeat()`
 * - **SSL/TLS**: 通过 `AMQP::Ssl` 类支持 TLS 加密通信
 *
 * 线程亲和性:
 * - **必须在主线程(Qt 事件循环所在线程)创建和销毁**
 *   原因: 内部使用 QTimer/QSocketNotifier,这些 Qt 对象有线程亲和性
 *   违反会导致跨线程 Qt 对象销毁崩溃
 * - 若需在工作线程使用,请通过 `QMetaObject::invokeMethod(backend, ...)`
 *   将操作转发到主线程执行
 * - 自动重连功能未实现,由上层 `RabbitMQConnection` 负责(v1.7.0)
 *
 * 编译启用:
 * ```bash
 * cmake -DENABLE_RABBITMQ=ON ...  # 仅 Linux/macOS
 * ```
 *
 * 适用场景:
 * - 生产环境(需要真实 RabbitMQ 服务器)
 * - 集成测试(配合 Docker Compose 启动 RabbitMQ)
 * - 性能基准测试
 *
 * 线程安全: 所有方法线程安全(ADR-005 Level 2),但需遵守线程亲和性约束。
 *
 * @warning 此类仅在 `SOUL_ENABLE_RABBITMQ` 宏定义时编译
 * @warning 仅支持 Linux/macOS(Windows Boost.Asio 后端计划 v1.8.0)
 * @see IAmqpBackend, InMemoryAmqpBackend
 * @since v1.7.0
 */
class AmqpCppBackend : public QObject, public IAmqpBackend {
    Q_OBJECT

public:
    AmqpCppBackend();
    ~AmqpCppBackend() override;

    // IAmqpBackend 接口实现
    Result<void> connect(const Config& config) override;
    void disconnect() override;
    [[nodiscard]] bool isConnected() const override;

    Result<void> declareExchange(const QString& name, ExchangeType type,
                                  bool durable = true, bool autoDelete = false) override;
    Result<QString> declareQueue(const QString& name, bool durable = true,
                                  bool exclusive = false, bool autoDelete = false) override;
    Result<void> bindQueue(const QString& queue, const QString& exchange,
                            const QString& routingKey) override;
    Result<void> unbindQueue(const QString& queue, const QString& exchange,
                              const QString& routingKey) override;

    Result<void> publish(const AmqpMessage& message) override;
    Result<void> consume(const QString& queue, AmqpConsumeCallback callback,
                          int prefetchCount = 1) override;
    Result<void> cancelConsume(const QString& queue) override;

    Result<void> ack(qint64 deliveryTag, bool multiple = false) override;
    Result<void> nack(qint64 deliveryTag, bool requeue = false, bool multiple = false) override;
    Result<void> reject(qint64 deliveryTag, bool requeue = false) override;

    void startConsuming() override;
    void stopConsuming() override;

private:
    /// @brief Qt 事件循环集成的 TCP Handler
    ///
    /// 继承 AMQP::TcpHandler,通过 QSocketNotifier 监听 amqpcpp 的
    /// 文件描述符,将网络事件分发到 Qt 主事件循环处理。
    class QtTcpHandler : public AMQP::TcpHandler {
    public:
        explicit QtTcpHandler(QObject* parent = nullptr);
        ~QtTcpHandler() override;

        // AMQP::TcpHandler 实现
        void monitor(AMQP::TcpConnection* conn, int fd, int flags) override;
        void onConnected(AMQP::TcpConnection* conn) override;
        void onError(AMQP::TcpConnection* conn, const char* message) override;
        void onClosed(AMQP::TcpConnection* conn) override;

        /// @brief 设置握手完成回调
        void setHandshakeCallback(std::function<void(bool, const std::string&)> cb) {
            m_handshakeCb = std::move(cb);
        }

    private:
        struct NotifierEntry {
            std::unique_ptr<QSocketNotifier> readNotifier;
            std::unique_ptr<QSocketNotifier> writeNotifier;
        };

        std::unordered_map<int, NotifierEntry> m_notifiers;
        AMQP::TcpConnection* m_conn = nullptr;
        QObject* m_parent = nullptr;
        std::function<void(bool, const std::string&)> m_handshakeCb;

        void processConnection(AMQP::TcpConnection* conn);
    };

    // === 内部状态 ===
    mutable std::mutex m_mutex;
    std::unique_ptr<QtTcpHandler> m_handler;
    std::shared_ptr<AMQP::TcpConnection> m_connection;
    std::shared_ptr<AMQP::TcpChannel> m_channel;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_consuming{false};

    // 消费者注册表: queue → (callback, consumerTag)
    struct ConsumerEntry {
        AmqpConsumeCallback callback;
        std::string consumerTag;
        int prefetchCount = 1;
    };
    std::unordered_map<QString, ConsumerEntry> m_consumers;

    // 心跳定时器(主线程亲和)
    QTimer* m_heartbeatTimer = nullptr;

    // 配置缓存(由上层 RabbitMQConnection 负责重连)
    Config m_config;

    // 握手状态(原子,无需 mutex)
    std::atomic<bool> m_handshakeDone{false};
    std::atomic<bool> m_handshakeSuccess{false};
    std::string m_handshakeError;

    // Channel 状态(channel 级别持久回调,使用成员变量避免 UAF)
    // onError 是持久回调,可能在 channel 生命周期内多次触发
    std::atomic<bool> m_channelReady{false};
    std::atomic<bool> m_channelError{false};
    std::string m_channelErrorMsg;  ///< 受 m_mutex 保护

    /// @brief Exchange 类型映射到 amqpcpp
    static AMQP::ExchangeType toAmqpExchangeType(ExchangeType type);

    /// @brief AmqpMessage 转换为 AMQP::Message
    static AMQP::Message toAmqpMessage(const AmqpMessage& message);

    /// @brief AMQP::Message 转换为 AmqpDelivery
    static AmqpDelivery fromAmqpMessage(const AMQP::Message& msg,
                                          qint64 deliveryTag,
                                          bool redelivered,
                                          const QString& exchange,
                                          const QString& routingKey,
                                          const QString& queue);

    /// @brief 等待握手完成(使用 QEventLoop 避免主线程死锁)
    /// @details 通过 QEventLoop 嵌套处理 Qt 事件,允许 QSocketNotifier
    ///          触发 amqpcpp 的 process(),完成 TCP+AMQP 握手回调
    /// @param timeout 超时时间
    /// @return 握手结果
    Result<void> waitForHandshake(std::chrono::milliseconds timeout);

    /// @brief 等待 atomic done 标志(使用 QEventLoop,锁外调用)
    /// @details 辅助方法:将异步 amqpcpp 操作的等待逻辑统一抽取。
    ///          调用者必须在锁外调用此方法,否则会导致死锁:
    ///          - QEventLoop::exec() 会处理 Qt 事件(QSocketNotifier/QTimer)
    ///          - 这些事件可能触发回调,回调中可能获取 m_mutex
    ///          - 若调用者持有 m_mutex,则死锁
    /// @param done 完成标志(由 amqpcpp 回调设置)
    /// @param timeout 超时时间
    /// @return true=完成,false=超时
    static bool waitForDone(const std::atomic<bool>& done,
                              std::chrono::milliseconds timeout);

    /// @brief 启动心跳定时器
    void startHeartbeat();

    /// @brief 停止心跳定时器
    void stopHeartbeat();

    /// @brief 生成唯一 consumerTag
    std::string generateConsumerTag(const QString& queue);
};

} // namespace mq
} // namespace sc

#endif // SOUL_ENABLE_RABBITMQ

#endif // SOUL_MQ_AMQPCPP_BACKEND_H
