#include <memory>
#include "soul/network/http_client.h"
#include "soul/network/http_request.h"
#include "soul/logging/log_macros.h"

int main() {
    auto client = std::make_shared<sc::network::HttpClient>();

    sc::network::HttpRequest request(sc::network::HttpMethod::Get, QUrl("https://httpbin.org/get"));
    request.addHeader("Accept", "application/json");

    auto result = client->send(request);

    if (result.isOk()) {
        const sc::network::HttpResponse& response = result.unwrap();
        SC_INFO("Status: " + QString::number(response.statusCode()).toStdString());
        SC_INFO("Body: " + response.text().toStdString());
    } else {
        SC_ERROR("Request failed: " + result.unwrapErr().message());
    }

    return 0;
}