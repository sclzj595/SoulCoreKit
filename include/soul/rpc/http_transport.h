#ifndef SOUL_RPC_HTTP_TRANSPORT_H
#define SOUL_RPC_HTTP_TRANSPORT_H

// ============================================================================
// http_transport.h — HTTP RPC Transport [v2.7.0 增强]
// ============================================================================
//
// v2.7.0 变更:
//   - 实现 IRpcCodec 接口 (通过 ISerializer 桥接)
//   - 实现 RpcTransportType::Http
//   - setCodec() 支持运行时切换编解码器
//
// 用法:
//   auto transport = std::make_shared<HttpTransport>("http://localhost:8080/rpc");
//   auto codec = std::make_shared<JsonRpcCodec>();
//   transport->setCodec(codec);
//   transport->start();
//   auto result = transport->sendRequest(req);

#include "soul/rpc/irpc_transport.h"
#include "soul/rpc/iserializer.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <atomic>
#include <memory>

namespace sc {
namespace rpc {

/// @brief JSON RPC Codec (默认) [v2.7.0 新增]
class JsonRpcCodec : public IRpcCodec {
public:
    QByteArray encodeRequest(const RpcRequest& request) override;
    QByteArray encodeResponse(const RpcResponse& response) override;
    Result<RpcRequest> decodeRequest(const QByteArray& data) override;
    Result<RpcResponse> decodeResponse(const QByteArray& data) override;
    std::string name() const override { return "JSON"; }
};

class HttpTransport : public QObject, public IRpcTransport {
    Q_OBJECT
public:
    explicit HttpTransport(const QString& baseUrl,
                          const QMap<QString, QString>& headers = {},
                          QObject* parent = nullptr);
    ~HttpTransport() override;

    // IRpcTransport
    Result<RpcResponse> sendRequest(const RpcRequest& request) override;
    void start() override;
    void stop() override;
    bool isRunning() const override;
    RpcTransportType type() const override { return RpcTransportType::Http; }
    void setCodec(std::shared_ptr<IRpcCodec> codec) override;

    // HttpTransport 特有
    void setSerializer(std::shared_ptr<ISerializer> serializer);
    std::shared_ptr<ISerializer> getSerializer() const;

    void setReadTimeout(int ms);
    void setHttp2Enabled(bool enabled);
    bool isHttp2Enabled() const;

private:
    QString m_baseUrl;
    QMap<QString, QString> m_headers;
    QNetworkAccessManager* m_manager;
    std::shared_ptr<ISerializer> m_serializer;
    std::shared_ptr<IRpcCodec> m_codec;  // v2.7.0
    int m_readTimeout = 30000;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_http2Enabled{true};
};

} // namespace rpc
} // namespace sc

#endif
