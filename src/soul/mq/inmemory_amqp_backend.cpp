#include "soul/mq/inmemory_amqp_backend.h"
#include "soul/logging/log_macros.h"
#include "soul/core/uuid.h"
#include <algorithm>
#include <chrono>
#include <regex>

namespace sc {
namespace mq {

InMemoryAmqpBackend::InMemoryAmqpBackend() = default;

InMemoryAmqpBackend::~InMemoryAmqpBackend() {
    // [v1.9.2] 确保 dispatch 线程先停止,再清理资源
    // 避免析构时 m_mutex 仍在被 dispatch 线程使用导致崩溃
    stopConsuming();
    m_connected.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_exchanges.clear();
        m_queues.clear();
        m_bindings.clear();
        m_consumers.clear();
        m_unacked.clear();
    }
}

Result<void> InMemoryAmqpBackend::connect(const Config& config) {
    (void)config;  // InMemory 后端不需要真实连接
    m_connected.store(true, std::memory_order_release);
    SC_INFO("InMemoryAmqpBackend: connected (in-memory mode)");
    return Result<void>::ok();
}

void InMemoryAmqpBackend::disconnect() {
    bool was_connected = m_connected.exchange(false, std::memory_order_acq_rel);
    if (!was_connected) {
        return;
    }
    stopConsuming();
    std::lock_guard<std::mutex> lock(m_mutex);
    m_exchanges.clear();
    m_queues.clear();
    m_bindings.clear();
    m_consumers.clear();
    m_unacked.clear();
    SC_INFO("InMemoryAmqpBackend: disconnected and cleared all state");
}

bool InMemoryAmqpBackend::isConnected() const {
    return m_connected.load(std::memory_order_acquire);
}

Result<void> InMemoryAmqpBackend::declareExchange(const QString& name, ExchangeType type,
                                                    bool durable, bool autoDelete) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }
    if (name.isEmpty()) {
        return Result<void>::err(Error(ErrorCode::InvalidArgument,
                                        "InMemoryAmqpBackend: exchange name must not be empty"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_exchanges.find(name) != m_exchanges.end()) {
        // 幂等声明:已存在则返回成功(AMQP 语义)
        return Result<void>::ok();
    }
    ExchangeInfo info;
    info.type = type;
    info.durable = durable;
    info.autoDelete = autoDelete;
    m_exchanges.emplace(name, std::move(info));
    SC_INFO("InMemoryAmqpBackend: declared exchange '" + name.toStdString() +
            "' type=" + std::to_string(static_cast<int>(type)));
    return Result<void>::ok();
}

Result<QString> InMemoryAmqpBackend::declareQueue(const QString& name, bool durable,
                                                    bool exclusive, bool autoDelete) {
    (void)durable;
    (void)exclusive;
    (void)autoDelete;
    if (!isConnected()) {
        return Error(ErrorCode::NotConnected, "InMemoryAmqpBackend: not connected");
    }

    QString queueName = name;
    if (queueName.isEmpty()) {
        queueName = generateQueueName();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queues.find(queueName) == m_queues.end()) {
        m_queues.emplace(queueName, std::deque<QueuedMessage>{});
    }
    // 幂等声明:已存在则返回成功
    return Result<QString>::ok(queueName);
}

Result<void> InMemoryAmqpBackend::bindQueue(const QString& queue, const QString& exchange,
                                              const QString& routingKey) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_exchanges.find(exchange) == m_exchanges.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
                                        "InMemoryAmqpBackend: exchange '" + exchange.toStdString() + "' not found"));
    }
    if (m_queues.find(queue) == m_queues.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
                                        "InMemoryAmqpBackend: queue '" + queue.toStdString() + "' not found"));
    }

    m_bindings.push_back({queue, exchange, routingKey});
    m_exchanges[exchange].boundQueues.insert(queue);
    return Result<void>::ok();
}

Result<void> InMemoryAmqpBackend::unbindQueue(const QString& queue, const QString& exchange,
                                                const QString& routingKey) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::remove_if(m_bindings.begin(), m_bindings.end(),
                              [&](const BindingInfo& b) {
                                  return b.queue == queue && b.exchange == exchange &&
                                         b.routingKey == routingKey;
                              });
    if (it == m_bindings.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
                                        "InMemoryAmqpBackend: binding not found"));
    }
    m_bindings.erase(it, m_bindings.end());

    // 检查 exchange 是否还有该队列的其他绑定
    bool hasOtherBinding = std::any_of(m_bindings.begin(), m_bindings.end(),
                                        [&](const BindingInfo& b) {
                                            return b.exchange == exchange && b.queue == queue;
                                        });
    if (!hasOtherBinding) {
        auto exIt = m_exchanges.find(exchange);
        if (exIt != m_exchanges.end()) {
            exIt->second.boundQueues.erase(queue);
        }
    }
    return Result<void>::ok();
}

Result<void> InMemoryAmqpBackend::publish(const AmqpMessage& message) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    routeMessage(message, message.exchange, message.routingKey);
    m_cv.notify_all();
    return Result<void>::ok();
}

Result<void> InMemoryAmqpBackend::consume(const QString& queue, AmqpConsumeCallback callback,
                                            int prefetchCount) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }
    if (!callback) {
        return Result<void>::err(Error(ErrorCode::InvalidArgument,
                                        "InMemoryAmqpBackend: consume callback must not be null"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_queues.find(queue) == m_queues.end()) {
        return Result<void>::err(Error(ErrorCode::NotFound,
                                        "InMemoryAmqpBackend: queue '" + queue.toStdString() + "' not found"));
    }
    ConsumerInfo info;
    info.callback = std::move(callback);
    info.prefetchCount = prefetchCount > 0 ? prefetchCount : std::numeric_limits<int>::max();
    info.unackedCount = 0;
    m_consumers[queue] = std::move(info);
    m_cv.notify_all();
    return Result<void>::ok();
}

Result<void> InMemoryAmqpBackend::cancelConsume(const QString& queue) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_consumers.erase(queue);
    return Result<void>::ok();
}

Result<void> InMemoryAmqpBackend::ack(qint64 deliveryTag, bool multiple) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    if (multiple) {
        // 批量确认:确认所有 <= deliveryTag 的未确认消息
        auto it = m_unacked.begin();
        while (it != m_unacked.end()) {
            if (it->first <= deliveryTag) {
                ConsumerInfo* ci = nullptr;
                auto cit = m_consumers.find(it->second.queue);
                if (cit != m_consumers.end()) {
                    ci = &cit->second;
                }
                it = m_unacked.erase(it);
                if (ci && ci->unackedCount > 0) {
                    ci->unackedCount--;
                }
            } else {
                ++it;
            }
        }
    } else {
        auto it = m_unacked.find(deliveryTag);
        if (it == m_unacked.end()) {
            return Result<void>::err(Error(ErrorCode::NotFound,
                                            "InMemoryAmqpBackend: delivery tag not found"));
        }
        QString queue = it->second.queue;
        m_unacked.erase(it);
        auto cit = m_consumers.find(queue);
        if (cit != m_consumers.end() && cit->second.unackedCount > 0) {
            cit->second.unackedCount--;
        }
    }
    m_cv.notify_all();
    return Result<void>::ok();
}

Result<void> InMemoryAmqpBackend::nack(qint64 deliveryTag, bool requeue, bool multiple) {
    if (!isConnected()) {
        return Result<void>::err(Error(ErrorCode::NotConnected,
                                        "InMemoryAmqpBackend: not connected"));
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto nackOne = [&](qint64 tag) {
        auto it = m_unacked.find(tag);
        if (it == m_unacked.end()) {
            return;
        }
        QString queue = it->second.queue;
        QueuedMessage qmsg = it->second.msg;
        m_unacked.erase(it);
        if (requeue) {
            // 重新入队(放到队首)
            auto qit = m_queues.find(queue);
            if (qit != m_queues.end()) {
                qmsg.deliveryTag = m_nextDeliveryTag.fetch_add(1, std::memory_order_relaxed);
                qit->second.push_front(std::move(qmsg));
            }
        }
        auto cit = m_consumers.find(queue);
        if (cit != m_consumers.end() && cit->second.unackedCount > 0) {
            cit->second.unackedCount--;
        }
    };

    if (multiple) {
        std::vector<qint64> tags;
        for (const auto& [tag, _] : m_unacked) {
            if (tag <= deliveryTag) {
                tags.push_back(tag);
            }
        }
        for (qint64 tag : tags) {
            nackOne(tag);
        }
    } else {
        nackOne(deliveryTag);
    }
    m_cv.notify_all();
    return Result<void>::ok();
}

Result<void> InMemoryAmqpBackend::reject(qint64 deliveryTag, bool requeue) {
    // reject 等价于 nack(requeue, multiple=false)
    return nack(deliveryTag, requeue, false);
}

void InMemoryAmqpBackend::startConsuming() {
    bool expected = false;
    if (!m_consuming.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        return;  // 已在消费中
    }
    // TSan-safe: 持 m_mutex 保护 m_dispatchThread 的写入,与 stopConsuming() 的读同步
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_dispatchThread = std::thread([this]() { this->dispatchLoop(); });
    }
    SC_INFO("InMemoryAmqpBackend: consuming started");
}

void InMemoryAmqpBackend::stopConsuming() {
    // [v1.9.2] 先设置停止标志,再通知唤醒,最后 join 线程
    // 必须在 join 之前设置标志,确保 dispatchLoop 能感知到停止
    m_consuming.store(false, std::memory_order_release);
    m_cv.notify_all();

    // TSan-safe: 持 m_mutex 保护 m_dispatchThread 的读写,与 startConsuming() 的写同步
    std::thread dispatchThread;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        dispatchThread = std::move(m_dispatchThread);
    }
    if (dispatchThread.joinable()) {
        dispatchThread.join();
    }
    SC_INFO("InMemoryAmqpBackend: consuming stopped");
}

void InMemoryAmqpBackend::dispatchLoop() {
    while (m_consuming.load(std::memory_order_acquire)) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&]() {
            return !m_consuming.load(std::memory_order_acquire) ||
                   std::any_of(m_queues.begin(), m_queues.end(),
                               [](const auto& kv) { return !kv.second.empty(); });
        });

        if (!m_consuming.load(std::memory_order_acquire)) {
            break;
        }

        // 遍历所有队列,将消息分发给消费者
        bool dispatched = false;
        for (auto& [queueName, queue] : m_queues) {
            // 注意:不持有 ConsumerInfo 引用跨 callback 调用
            // 回调中可能调用 cancelConsume 导致迭代器/引用失效(UAF)
            auto cit = m_consumers.find(queueName);
            if (cit == m_consumers.end()) {
                continue;  // 无消费者
            }

            // 拷贝回调,避免持有引用(PrefetchCount/unackedCount 实时读取)
            AmqpConsumeCallback callback = cit->second.callback;
            int prefetchCount = cit->second.prefetchCount;

            while (!queue.empty()) {
                // 每次循环重新检查 consumer 是否仍存在(回调可能已 cancel)
                cit = m_consumers.find(queueName);
                if (cit == m_consumers.end()) {
                    break;  // consumer 已被 cancel
                }
                if (cit->second.unackedCount >= prefetchCount) {
                    break;  // QoS 限制
                }

                QueuedMessage qmsg = queue.front();
                queue.pop_front();

                qint64 tag = m_nextDeliveryTag.fetch_add(1, std::memory_order_relaxed);
                qmsg.deliveryTag = tag;

                AmqpDelivery delivery;
                delivery.deliveryTag = tag;
                delivery.redelivered = false;
                delivery.exchange = qmsg.exchange;
                delivery.routingKey = qmsg.routingKey;
                delivery.queue = queueName;
                delivery.message = qmsg.message;

                m_unacked[tag] = {tag, queueName, qmsg};
                cit->second.unackedCount++;
                dispatched = true;

                // 回调在锁外执行,避免死锁(回调可能调用 ack/nack/cancelConsume)
                // 注意:回调返回后必须重新查找 consumer,不能再用旧引用
                lock.unlock();
                callback(delivery);
                lock.lock();
            }
        }

        // 如果没有分发任何消息,短暂等待避免忙等
        // (当 prefetchCount 已满但队列非空时可能发生)
        // 使用无谓词 wait_for:既可被 ack/nack 的 notify_all 提前唤醒,
        // 又保证至少让出 CPU 10ms,不会因谓词恒 true 而忙等
        if (!dispatched) {
            m_cv.wait_for(lock, std::chrono::milliseconds(10));
        }
    }
}

void InMemoryAmqpBackend::routeMessage(const AmqpMessage& message,
                                         const QString& exchangeName,
                                         const QString& routingKey) {
    // 注意:调用者必须持有 m_mutex
    auto exIt = m_exchanges.find(exchangeName);
    if (exIt == m_exchanges.end()) {
        // exchange 不存在:消息丢弃(AMQP 默认行为,除非 mandatory=true)
        return;
    }

    const ExchangeInfo& ex = exIt->second;
    std::vector<QString> targetQueues;

    switch (ex.type) {
    case ExchangeType::Direct:
        // 精确匹配 routing key
        for (const auto& b : m_bindings) {
            if (b.exchange == exchangeName && b.routingKey == routingKey) {
                targetQueues.push_back(b.queue);
            }
        }
        break;
    case ExchangeType::Fanout:
        // 广播到所有绑定的队列
        for (const auto& queueName : ex.boundQueues) {
            targetQueues.push_back(queueName);
        }
        break;
    case ExchangeType::Topic:
        // 模式匹配
        for (const auto& b : m_bindings) {
            if (b.exchange == exchangeName && matchTopicPattern(b.routingKey, routingKey)) {
                targetQueues.push_back(b.queue);
            }
        }
        break;
    case ExchangeType::Headers:
        // Headers 类型暂未实现
        SC_WARN("InMemoryAmqpBackend: Headers exchange type not implemented, message dropped");
        break;
    }

    // 投递到目标队列
    for (const auto& queueName : targetQueues) {
        auto qIt = m_queues.find(queueName);
        if (qIt != m_queues.end()) {
            QueuedMessage qmsg;
            qmsg.deliveryTag = 0;  // 实际 tag 在分发时分配
            qmsg.message = message;
            qmsg.exchange = exchangeName;
            qmsg.routingKey = routingKey;
            qIt->second.push_back(std::move(qmsg));
        }
    }
}

bool InMemoryAmqpBackend::matchTopicPattern(const QString& pattern, const QString& key) {
    // AMQP 0.9.1 Topic 匹配:
    // - '*' 匹配一个词(不含 '.')
    // - '#' 匹配零个或多个词
    // - routing key 和 pattern 都以 '.' 分隔
    if (pattern == key) {
        return true;
    }
    if (pattern.isEmpty() || key.isEmpty()) {
        return false;
    }

    QStringList patternParts = pattern.split('.', Qt::KeepEmptyParts);
    QStringList keyParts = key.split('.', Qt::KeepEmptyParts);

    // 使用 qsizetype 与 QStringList::size() 返回类型一致,避免 sign-compare 警告
    qsizetype pi = 0;
    qsizetype ki = 0;

    while (pi < patternParts.size() && ki < keyParts.size()) {
        const QString& p = patternParts[pi];
        if (p == "#") {
            // '#' 匹配零个或多个词
            if (pi == patternParts.size() - 1) {
                return true;  // '#' 在末尾,匹配剩余所有
            }
            // 尝试匹配后续 pattern
            const QString& nextP = patternParts[pi + 1];
            while (ki < keyParts.size() && keyParts[ki] != nextP) {
                ki++;
            }
            if (ki >= keyParts.size()) {
                return false;
            }
            pi++;
            ki++;
        } else if (p == "*") {
            // '*' 匹配一个词
            pi++;
            ki++;
        } else if (p == keyParts[ki]) {
            pi++;
            ki++;
        } else {
            return false;
        }
    }

    // 处理 pattern 末尾的 '#'
    if (pi < patternParts.size() && patternParts[pi] == "#") {
        pi++;
    }

    return pi == patternParts.size() && ki == keyParts.size();
}

QString InMemoryAmqpBackend::generateQueueName() {
    return QString("amq.gen-") + QString::fromStdString(Uuid::generate()).left(22);
}

} // namespace mq
} // namespace sc
