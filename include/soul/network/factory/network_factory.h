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

namespace sc::network {

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

    std::shared_ptr<INetwork> create(const QUrl& url) {
        Protocol protocol = detectProtocol(url);
        return create(protocol);
    }

    std::shared_ptr<INetwork> create(Protocol protocol) {
        switch (protocol) {
        case Protocol::HTTP:
        case Protocol::HTTPS:
            return std::make_shared<HttpClientAdapter>();
        case Protocol::TCP:
            return std::make_shared<TcpClientAdapter>();
        case Protocol::WebSocket:
        case Protocol::WSS:
            return std::make_shared<WsClientAdapter>();
        case Protocol::MQTT:
        case Protocol::MQTTS:
            return std::make_shared<MqttClientAdapter>();
        case Protocol::Bluetooth:
            return std::make_shared<BluetoothClientAdapter>();
        case Protocol::SerialPort:
            return std::make_shared<SerialPortAdapter>();
        case Protocol::NamedPipe:
            return std::make_shared<NamedPipeAdapter>();
        default:
            return std::make_shared<HttpClientAdapter>();
        }
    }

private:
    Protocol detectProtocol(const QUrl& url) {
        QString scheme = url.scheme().toLower();
        if (scheme == "http") return Protocol::HTTP;
        if (scheme == "https") return Protocol::HTTPS;
        if (scheme == "tcp") return Protocol::TCP;
        if (scheme == "ws") return Protocol::WebSocket;
        if (scheme == "wss") return Protocol::WSS;
        if (scheme == "mqtt") return Protocol::MQTT;
        if (scheme == "mqtts") return Protocol::MQTTS;
        if (scheme == "bluetooth") return Protocol::Bluetooth;
        if (scheme == "serial") return Protocol::SerialPort;
        if (scheme == "pipe") return Protocol::NamedPipe;
        return Protocol::HTTP;
    }
};

}

#endif
