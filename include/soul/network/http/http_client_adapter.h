#ifndef SOUL_NETWORK_HTTP_HTTP_CLIENT_ADAPTER_H
#define SOUL_NETWORK_HTTP_HTTP_CLIENT_ADAPTER_H

#include <memory>
#include "soul/network/core/network_adapter_base.h"
#include "soul/network/http_client.h"
#include "soul/network/http_request.h"

namespace sc::network {

class HttpClientAdapter : public NetworkAdapterBase {
    Q_OBJECT
public:
    explicit HttpClientAdapter(QObject* parent = nullptr);
    ~HttpClientAdapter() override;

private:
    Result<NetworkMessage> doSend(const NetworkMessage& message) override;
    void doSendAsync(const NetworkMessage& message, ResponseCallback callback) override;
    void doConnect(const QUrl& url) override;
    void doDisconnect() override;
    bool doIsConnected() const override;

    HttpRequest convertToHttpRequest(const NetworkMessage& msg);
    NetworkMessage convertToNetworkMessage(const HttpResponse& response);

    std::unique_ptr<HttpClient> m_client;
};

}

#endif