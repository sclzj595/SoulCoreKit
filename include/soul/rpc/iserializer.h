#ifndef SOUL_RPC_ISERIALIZER_H
#define SOUL_RPC_ISERIALIZER_H

// ============================================================================
// iserializer.h — RPC 序列化接口 (v1.9.2: 迁移到 nlohmann/json)
// ============================================================================

#include <QString>
#include <QByteArray>
#include <memory>
#include <variant>

#include "soul/core/result.h"
#include "soul/utils/json/json_helper.h"

namespace sc { namespace rpc {

using RpcValue = std::variant<std::monostate, qint64, double, bool, QString, QByteArray>;

class ISerializer {
public:
    virtual ~ISerializer() = default;
    virtual QByteArray serialize(const sc::json::Json& obj) const = 0;
    virtual Result<sc::json::Json> deserialize(const QByteArray& data) const = 0;
    virtual QByteArray serializeValue(const RpcValue& value) const = 0;
    virtual Result<RpcValue> deserializeValue(const QByteArray& data) const = 0;
    virtual QString contentType() const = 0;
};

class JsonSerializer : public ISerializer {
public:
    QByteArray serialize(const sc::json::Json& obj) const override;
    Result<sc::json::Json> deserialize(const QByteArray& data) const override;
    QByteArray serializeValue(const RpcValue& value) const override;
    Result<RpcValue> deserializeValue(const QByteArray& data) const override;
    QString contentType() const override { return "application/json"; }
};

}}
#endif