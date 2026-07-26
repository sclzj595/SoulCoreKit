#ifndef SOUL_MQ_INMEMORY_AMQP_BACKEND_H
#define SOUL_MQ_INMEMORY_AMQP_BACKEND_H

#include <QHash>
#include <QMutex>
#include <QString>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "soul/mq/iamqp_backend.h"

namespace sc {
namespace mq {

/**
 * @class InMemoryAmqpBackend
 * @brief 基于 STL 内存队列的 AMQP 后端实现
 *
 * InMemoryAmqpBackend 严格模拟 AMQP 0.9.1 协议语义,包括:
 * - 四种 exchange 类型(Direct/Fanout/Topic/Headers)
 * - Topic exchange 的 routing key 通配符匹配(* 和 #)
 * - 队列声明/绑定/解绑
 * - 消息发布与路由
 * - 消费者订阅与消息分发
 * - QoS 预取计数(prefetchCount)
 * - 消息确认(ack/nack/reject)与重新入队
 * - 自动生成队列名(当传入空字符串时)
 *
 * 适用场景:
 * - 单元测试(无需 RabbitMQ 服务器)
 * - 本地开发环境
 * - CI/CD 流水线(无外部依赖)
 * - 功能验证与原型开发
 *
 * 线程安全: 所有方法线程安全(ADR-005 Level 2)。
 *
 * @see IAmqpBackend, AmqpCppBackend
 * @since v1.7.0
 */
class InMemoryAmqpBackend : public IAmqpBackend {
public:
    InMemoryAmqpBackend();
    ~InMemoryAmqpBackend() override;

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
    /// @brief Exchange 元数据
    struct ExchangeInfo {
        ExchangeType type = ExchangeType::Direct;
        bool durable = true;
        bool autoDelete = false;
        std::unordered_set<QString> boundQueues;  ///< 绑定的队列名
    };

    /// @brief Binding 元数据
    struct BindingInfo {
        QString queue;
        QString exchange;
        QString routingKey;
    };

    /// @brief 队列中的待消费消息
    struct QueuedMessage {
        qint64 deliveryTag = 0;
        AmqpMessage message;
        QString exchange;
        QString routingKey;
    };

    /// @brief 消费者订阅信息
    struct ConsumerInfo {
        AmqpConsumeCallback callback;
        int prefetchCount = 1;
        int unackedCount = 0;  ///< 未确认消息数(用于 QoS)
    };

    /// @brief 未确认消息(等待 ack/nack)
    struct UnackedMessage {
        qint64 deliveryTag = 0;
        QString queue;
        QueuedMessage msg;
    };

    // === 内部状态(均受 m_mutex 保护) ===
    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<bool> m_connected{false};
    std::atomic<bool> m_consuming{false};
    std::atomic<qint64> m_nextDeliveryTag{1};

    std::unordered_map<QString, ExchangeInfo> m_exchanges;
    std::unordered_map<QString, std::deque<QueuedMessage>> m_queues;
    std::vector<BindingInfo> m_bindings;
    std::unordered_map<QString, ConsumerInfo> m_consumers;
    std::unordered_map<qint64, UnackedMessage> m_unacked;

    std::thread m_dispatchThread;

    /// @brief 分发线程主循环
    void dispatchLoop();

    /// @brief 路由消息到匹配的队列
    /// @param message 待路由的消息
    /// @param exchangeName 目标 exchange
    /// @param routingKey 路由键
    void routeMessage(const AmqpMessage& message,
                      const QString& exchangeName,
                      const QString& routingKey);

    /// @brief 检查 routing key 是否匹配 binding pattern(Topic 类型)
    /// @param pattern 绑定模式(支持 * 和 #)
    /// @param key 实际 routing key
    [[nodiscard]] static bool matchTopicPattern(const QString& pattern, const QString& key);

    /// @brief 生成唯一队列名
    [[nodiscard]] static QString generateQueueName();
};

} // namespace mq
} // namespace sc

#endif // SOUL_MQ_INMEMORY_AMQP_BACKEND_H
