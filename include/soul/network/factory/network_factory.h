/**
 * @file factory/network_factory.h
 * @brief 网络工厂类
 * @details 工厂模式，根据 URL 协议创建对应的网络客户端
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_FACTORY_NETWORK_FACTORY_H
#define SOUL_NETWORK_FACTORY_NETWORK_FACTORY_H

#include <memory>
#include <QUrl>
#include "soul/core/singleton.h"
#include "soul/network/core/inetwork.h"
#include "soul/network/http/http_client_adapter.h"
#include "soul/network/tcp/tcp_client_adapter.h"
#include "soul/network/websocket/ws_client_adapter.h"
#include "soul/network/mqtt/mqtt_client_adapter.h"
#include "soul/network/bluetooth/bluetooth_client_adapter.h"
#include "soul/network/serial/serial_port_adapter.h"
#include "soul/network/namedpipe/named_pipe_adapter.h"

namespace sc {
namespace network {

class NetworkFactory : public Singleton<NetworkFactory> {
public:
    enum class Protocol {
        HTTP,
        HTTPS,
        TCP,
        WebSocket,
        WSS,
        MQTT,
        MQTTS,
        Bluetooth,
        SerialPort,
        NamedPipe
    };

    std::shared_ptr<INetwork> create(const QUrl& url);
    std::shared_ptr<INetwork> create(Protocol protocol);
    Protocol detectProtocol(const QUrl& url);
    std::string protocolToString(Protocol protocol) const;

private:
    void registerDefaultProtocols();
};

} // namespace network
} // namespace sc

#endif
