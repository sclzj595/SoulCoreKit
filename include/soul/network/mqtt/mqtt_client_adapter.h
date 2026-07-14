#ifndef SOUL_NETWORK_MQTT_MQTT_CLIENT_ADAPTER_H
#define SOUL_NETWORK_MQTT_MQTT_CLIENT_ADAPTER_H

#include "soul/network/core/network_adapter_base.h"

namespace sc::network {

class MqttClientAdapter : public NetworkAdapterBase {
    Q_OBJECT
public:
    MqttClientAdapter();
    
protected:
    Result<NetworkMessage> doSend(const NetworkMessage& message) override;
    void doSendAsync(const NetworkMessage& message, ResponseCallback callback) override;
    void doConnect(const QUrl& url) override;
    void doDisconnect() override;
    bool doIsConnected() const override;
    
private:
    bool m_connected = false;
};

}

#endif