#ifndef SOUL_RPC_CLIENT_PROXY_H
#define SOUL_RPC_CLIENT_PROXY_H

// ============================================================================
// client_proxy.h — RPC 客户端代理 (v1.9.2: 迁移到 nlohmann/json)
// ============================================================================

#include <QString>
#include <memory>
#include <functional>

#include "soul/core/result.h"
#include "soul/rpc/irpc_transport.h"
#include "soul/rpc/iserializer.h"
#include "soul/utils/json/json_helper.h"

namespace sc { namespace rpc {

class ClientProxy {
public:
    ClientProxy(std::shared_ptr<IRpcTransport> transport,
                std::shared_ptr<ISerializer> serializer);

    Result<sc::json::Json> call(const QString& service, const QString& method,
                                const sc::json::Json& params = sc::json::Json::object());

    template<typename T>
    Result<T> callAndParse(const QString& service, const QString& method,
                            const sc::json::Json& params = sc::json::Json::object()) {
        auto result = call(service, method, params);
        if (result.isErr()) return result.unwrapErr();
        sc::json::Json data = result.unwrap();
        if (!data.contains("result")) {
            return Result<T>(Error(ErrorCode::InternalError,
                "Response missing 'result' field"));
        }
        sc::json::Json& val = data["result"];

        try {
            if constexpr (std::is_same_v<T, QString>) {
                return Result<T>(QString::fromStdString(val.get<std::string>()));
            } else if constexpr (std::is_same_v<T, qint64>) {
                return Result<T>(val.get<qint64>());
            } else if constexpr (std::is_same_v<T, int>) {
                return Result<T>(val.get<int>());
            } else if constexpr (std::is_same_v<T, double>) {
                return Result<T>(val.get<double>());
            } else if constexpr (std::is_same_v<T, bool>) {
                return Result<T>(val.get<bool>());
            } else if constexpr (std::is_same_v<T, sc::json::Json>) {
                return Result<T>(val);
            } else {
                return Result<T>(Error(ErrorCode::InternalError,
                    "Unsupported target type in callAndParse"));
            }
        } catch (const sc::json::Json::exception& e) {
            return Result<T>(Error(ErrorCode::InternalError,
                QString::fromStdString(e.what())));
        }
    }

private:
    std::shared_ptr<IRpcTransport> m_transport;
    std::shared_ptr<ISerializer> m_serializer;
    QString generateRequestId();
};

}}
#endif