#ifndef SOUL_MQ_IAMQP_BACKEND_H
#define SOUL_MQ_IAMQP_BACKEND_H

#include <QString>
#include <QByteArray>
#include <functional>
#include <memory>
#include <string>
#include "soul/core/result.h"
#include "soul/core/interface.h"

namespace sc {
namespace mq {

/// @brief AMQP exchange 类型(对应 AMQP 0.9.1 规范)
enum class ExchangeType {
    Direct,   ///< 直连:routing key 精确匹配
    Fanout,   ///< 扇出:广播到所有绑定队列
    Topic,    ///< 主题:routing key 模式匹配(支持 * 和 # 通配符)
    Headers   ///< 头部:按消息头属性匹配(暂未实现,预留)
};

/// @brief AMQP 消息属性
struct AmqpMessage {
    QString exchange;        ///< 目标 exchange
    QString routingKey;      ///< 路由键
    QByteArray body;         ///< 消息体
    QString messageId;       ///< 消息 ID
    QString correlationId;   ///< 关联 ID
    QString replyTo;         ///< 回复队列
    int deliveryMode = 2;    ///< 1=非持久,2=持久
    int priority = 0;        ///< 优先级 0-9
    QString contentType;     ///< 内容类型(如 application/json)
    QString contentEncoding; ///< 内容编码(如 utf-8)
    qint64 timestamp = 0;    ///< 时间戳(毫秒)
};

/// @brief 消费到的消息(含 delivery tag 用于 ack/nack)
struct AmqpDelivery {
    qint64 deliveryTag = 0;  ///< 投递标签(用于 ack/nack)
    bool redelivered = false; ///< 是否为重新投递
    QString exchange;        ///< 来源 exchange
    QString routingKey;      ///< 路由键
    QString queue;           ///< 来源队列
    AmqpMessage message;     ///< 消息内容
};

/// @brief 消费回调函数类型
using AmqpConsumeCallback = std::function<void(const AmqpDelivery&)>;

/**
 * @class IAmqpBackend
 * @brief AMQP 后端抽象接口
 *
 * IAmqpBackend 定义了 AMQP 0.9.1 协议的核心操作抽象,为 RabbitMQ 真实集成
 * 提供可插拔后端能力。实现类必须保证线程安全(ADR-005 Level 2)。
 *
 * 设计目标:
 * - 解耦协议层与业务层:RabbitMQConnection/Producer/Consumer 委托至此接口
 * - 支持多后端:InMemoryAmqpBackend(开发/测试) + AmqpCppBackend(生产)
 * - 公共接口零变更:IMQConnection/IMQProducer/IMQConsumer 签名不变
 *
 * @see InMemoryAmqpBackend, AmqpCppBackend
 * @since v1.7.0
 */
class IAmqpBackend : public IInterface {
public:
    /// @brief 连接配置
    struct Config {
        QString host = "localhost";
        int port = 5672;
        QString username = "guest";
        QString password = "guest";
        QString virtualHost = "/";
        int connectionTimeout = 30000;  ///< 连接超时(毫秒)
        int heartbeatInterval = 60;     ///< 心跳间隔(秒)
        int maxChannels = 10;           ///< 最大通道数
        bool enableSsl = false;         ///< 是否启用 SSL/TLS
        QString caCertPath;             ///< CA 证书路径
        QString clientCertPath;         ///< 客户端证书路径
        QString clientKeyPath;          ///< 客户端私钥路径
    };

    ~IAmqpBackend() override = default;

    /// @brief 建立连接
    virtual Result<void> connect(const Config& config) = 0;

    /// @brief 断开连接
    virtual void disconnect() = 0;

    /// @brief 是否已连接
    [[nodiscard]] virtual bool isConnected() const = 0;

    /// @brief 声明 exchange
    /// @param name exchange 名称
    /// @param type exchange 类型
    /// @param durable 是否持久化
    /// @param autoDelete 无队列绑定时自动删除
    virtual Result<void> declareExchange(const QString& name,
                                          ExchangeType type,
                                          bool durable = true,
                                          bool autoDelete = false) = 0;

    /// @brief 声明队列
    /// @param name 队列名称(空字符串表示由服务器自动生成)
    /// @param durable 是否持久化
    /// @param exclusive 是否排他(仅当前连接可用)
    /// @param autoDelete 无消费者时自动删除
    /// @return 队列名称(若传入空字符串则返回生成的名称)
    virtual Result<QString> declareQueue(const QString& name,
                                          bool durable = true,
                                          bool exclusive = false,
                                          bool autoDelete = false) = 0;

    /// @brief 绑定队列到 exchange
    /// @param queue 队列名称
    /// @param exchange exchange 名称
    /// @param routingKey 绑定键(Topic 类型支持通配符)
    virtual Result<void> bindQueue(const QString& queue,
                                    const QString& exchange,
                                    const QString& routingKey) = 0;

    /// @brief 解绑队列
    virtual Result<void> unbindQueue(const QString& queue,
                                      const QString& exchange,
                                      const QString& routingKey) = 0;

    /// @brief 发布消息
    virtual Result<void> publish(const AmqpMessage& message) = 0;

    /// @brief 订阅队列(设置消费回调)
    /// @param queue 队列名称
    /// @param callback 消费回调
    /// @param prefetchCount QoS 预取计数(0=无限制)
    virtual Result<void> consume(const QString& queue,
                                  AmqpConsumeCallback callback,
                                  int prefetchCount = 1) = 0;

    /// @brief 取消订阅
    virtual Result<void> cancelConsume(const QString& queue) = 0;

    /// @brief 确认消息(ack)
    /// @param deliveryTag 投递标签
    /// @param multiple 是否批量确认(确认所有 <= deliveryTag 的消息)
    virtual Result<void> ack(qint64 deliveryTag, bool multiple = false) = 0;

    /// @brief 拒绝消息(nack)
    /// @param deliveryTag 投递标签
    /// @param requeue 是否重新入队
    /// @param multiple 是否批量拒绝
    virtual Result<void> nack(qint64 deliveryTag, bool requeue = false, bool multiple = false) = 0;

    /// @brief 拒绝消息(reject,与 nack 类似但不支持 multiple)
    virtual Result<void> reject(qint64 deliveryTag, bool requeue = false) = 0;

    /// @brief 开始消费(启动分发循环)
    virtual void startConsuming() = 0;

    /// @brief 停止消费
    virtual void stopConsuming() = 0;

    std::string interfaceName() const override { return "IAmqpBackend"; }
};

} // namespace mq
} // namespace sc

#endif // SOUL_MQ_IAMQP_BACKEND_H
