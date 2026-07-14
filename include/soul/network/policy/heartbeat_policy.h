#ifndef SOUL_NETWORK_POLICY_HEARTBEAT_POLICY_H
#define SOUL_NETWORK_POLICY_HEARTBEAT_POLICY_H

#include <QTimer>
#include <memory>
#include <QObject>
#include "inetwork_policy.h"
#include "soul/network/core/inetwork.h"

namespace sc::network {

class HeartbeatPolicy : public QObject, public INetworkPolicy {
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

}

#endif