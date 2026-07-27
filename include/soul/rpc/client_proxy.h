#ifndef SOUL_RPC_CLIENT_PROXY_H
#define SOUL_RPC_CLIENT_PROXY_H
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QVariant>
#include <memory>
#include <functional>
#include "soul/core/result.h"
#include "soul/rpc/irpc_transport.h"
#include "soul/rpc/iserializer.h"
namespace sc { namespace rpc {
class ClientProxy {
public:
    ClientProxy(std::shared_ptr<IRpcTransport> transport,
                std::shared_ptr<ISerializer> serializer);
    Result<QJsonObject> call(const QString& service, const QString& method,
                              const QJsonObject& params = QJsonObject());
    template<typename T>
    Result<T> callAndParse(const QString& service, const QString& method,
                            const QJsonObject& params = QJsonObject()) {
        auto result = call(service, method, params);
        if (result.isErr()) return result.unwrapErr();
        QJsonObject data = result.unwrap();
        QJsonValue val = data["result"];
        if constexpr (std::is_same_v<T, QString>) {
            return Result<T>(val.toString());
        } else if constexpr (std::is_same_v<T, qint64>) {
            return Result<T>(val.toVariant().toLongLong());
        } else if constexpr (std::is_same_v<T, int>) {
            return Result<T>(val.toInt());
        } else if constexpr (std::is_same_v<T, double>) {
            return Result<T>(val.toDouble());
        } else if constexpr (std::is_same_v<T, bool>) {
            return Result<T>(val.toBool());
        } else if constexpr (std::is_same_v<T, QJsonObject>) {
            return Result<T>(val.toObject());
        } else if constexpr (std::is_same_v<T, QJsonArray>) {
            return Result<T>(val.toArray());
        } else {
            auto v = val.toVariant();
            // qvariant_cast<T> has no bool* out-param overload; use canConvert+value.
            if (!v.canConvert<T>()) {
                return Result<T>(Error(ErrorCode::InternalError,
                    QString("Failed to convert QVariant to target type")));
            }
            return Result<T>(v.value<T>());
        }
    }
private:
    std::shared_ptr<IRpcTransport> m_transport;
    std::shared_ptr<ISerializer> m_serializer;
    QString generateRequestId();
};
}}
#endif