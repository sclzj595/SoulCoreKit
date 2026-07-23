#include "soul/network/http/http_client_adapter.h"
#include "soul/network/http_response.h"
#include "soul/network/http_request.h"

namespace sc {
namespace network {

HttpClientAdapter::HttpClientAdapter(QObject* parent)
    : NetworkAdapterBase(parent), m_client(std::make_unique<HttpClient>()) {}

HttpClientAdapter::~HttpClientAdapter() = default;

void HttpClientAdapter::doConnect(const QUrl& url) {
    Q_UNUSED(url);
    updateState(NetworkState::Connected);
    emit connected();
}

void HttpClientAdapter::doDisconnect() {
    updateState(NetworkState::Disconnected);
}

bool HttpClientAdapter::doIsConnected() const {
    return state() == NetworkState::Connected;
}

Result<NetworkMessage> HttpClientAdapter::doSend(const NetworkMessage& message) {
    auto httpRequest = convertToHttpRequest(message);
    auto result = m_client->send(httpRequest);
    if (result.isErr()) {
        return result.unwrapErr();
    }
    return convertToNetworkMessage(result.unwrap());
}

void HttpClientAdapter::doSendAsync(const NetworkMessage& message, ResponseCallback callback) {
    auto httpRequest = convertToHttpRequest(message);
    m_client->sendAsync(httpRequest, [this, callback](const Result<HttpResponse>& result) {
        if (result.isErr()) {
            callback(result.unwrapErr());
        } else {
            callback(convertToNetworkMessage(result.unwrap()));
        }
    });
}

HttpRequest HttpClientAdapter::convertToHttpRequest(const NetworkMessage& msg) {
    HttpRequest request(HttpMethod::Get, msg.api.isEmpty() ? msg.api : msg.api);
    if (!msg.api.isEmpty()) {
        request.setUrl(QUrl(msg.api));
    }
    request.setBody(msg.payload);
    for (const auto& key : msg.metadata.keys()) {
        request.addHeader(key, msg.metadata[key].toString());
    }
    if (msg.metadata.contains("timeout")) {
        request.setTimeout(msg.metadata["timeout"].toInt());
    }
    return request;
}

NetworkMessage HttpClientAdapter::convertToNetworkMessage(const HttpResponse& response) {
    NetworkMessage msg;
    msg.statusCode = response.statusCode();
    msg.payload = response.body();
    msg.message = response.isSuccess() ? "Success" : response.errorString();
    for (const auto& key : response.headers().keys()) {
        msg.metadata[key] = response.headers()[key];
    }
    return msg;
}

} // namespace network
} // namespace sc