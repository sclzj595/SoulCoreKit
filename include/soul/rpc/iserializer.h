#ifndef SOUL_RPC_ISERIALIZER_H
#define SOUL_RPC_ISERIALIZER_H
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QByteArray>
#include <memory>
#include <variant>
#include "soul/core/result.h"
namespace sc { namespace rpc {
using RpcValue = std::variant<std::monostate, qint64, double, bool, QString, QByteArray>;
class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual QByteArray serialize(const QJsonObject& obj) const = 0;
    virtual Result<QJsonObject> deserialize(const QByteArray& data) const = 0;
    virtual QByteArray serializeValue(const RpcValue& value) const = 0;
    virtual RpcValue deserializeValue(const QByteArray& data) const = 0;
    virtual QString contentType() const = 0;
};
class JsonSerializer : public ISerializer {
public:
    QByteArray serialize(const QJsonObject& obj) const override;
    Result<QJsonObject> deserialize(const QByteArray& data) const override;
    QByteArray serializeValue(const RpcValue& value) const override;
    RpcValue deserializeValue(const QByteArray& data) const override;
    QString contentType() const override { return "application/json"; }
};
}}
#endif