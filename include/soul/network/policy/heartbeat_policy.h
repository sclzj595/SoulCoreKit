/**
 * @file policy/heartbeat_policy.h
 * @brief 心跳策略类
 * @details 管理网络连接的心跳检测和保活机制
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_POLICY_HEARTBEAT_POLICY_H
#define SOUL_NETWORK_POLICY_HEARTBEAT_POLICY_H

#include <QTimer>
#include <memory>
#include <QObject>
#include "soul/network/network_global.h"
#include "inetwork_policy.h"
#ifndef Q_MOC_RUN
#include "soul/network/core/inetwork.h"
#endif

namespace sc {
namespace network {

class SC_NETWORK_EXPORT HeartbeatPolicy : public QObject, public INetworkPolicy {
    Q_OBJECT
public:
    HeartbeatPolicy(int intervalMs = 30000, int timeoutMs = 10000);
    ~HeartbeatPolicy() override;
    
    void apply(NetworkMessage& message) override;
    
    void start(std::shared_ptr<INetwork> network);
    void stop();
    
    int interval() const;
    HeartbeatPolicy& setInterval(int ms);
    
    int timeout() const;
    HeartbeatPolicy& setTimeout(int ms);
    
    void setHeartbeatMessage(const NetworkMessage& message);
    NetworkMessage heartbeatMessage() const;
    
signals:
    void heartbeatTimeout();
    
private:
    int m_interval = 30000;
    int m_timeout = 10000;
    QTimer m_timer;
    QTimer m_timeoutTimer;
    NetworkMessage m_heartbeatMessage;
    std::weak_ptr<INetwork> m_network;
    
    void sendHeartbeat();
    void onResponse();
};

} // namespace network
} // namespace sc

#endif