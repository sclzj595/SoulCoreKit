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
        SC_INFO_FMT("Status: {}", response.statusCode());
        SC_INFO_FMT("Body: {}", response.text().toStdString());
    } else {
        SC_ERROR_FMT("Request failed: {}", result.unwrapErr().message().toStdString());
    }

    return 0;
}