#ifndef SOUL_RPC_HTTP_TRANSPORT_H
#define SOUL_RPC_HTTP_TRANSPORT_H

#include "soul/rpc/irpc_transport.h"
#include "soul/rpc/iserializer.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <memory>

namespace sc {
namespace rpc {

class HttpTransport : public QObject, public IRpcTransport {
    Q_OBJECT
public:
    explicit HttpTransport(const QString& baseUrl,
                          const QMap<QString, QString>& headers = {},
                          QObject* parent = nullptr);
    ~HttpTransport() override;

    Result<RpcResponse> sendRequest(const RpcRequest& request) override;
    void start() override;
    void stop() override;
    bool isRunning() const override;

    void setSerializer(std::shared_ptr<ISerializer> serializer);
    std::shared_ptr<ISerializer> getSerializer() const;

    void setConnectTimeout(int ms);
    void setReadTimeout(int ms);

private:
    QString m_baseUrl;
    QMap<QString, QString> m_headers;
    QNetworkAccessManager* m_manager;
    std::shared_ptr<ISerializer> m_serializer;
    int m_connectTimeout = 5000;
    int m_readTimeout = 30000;
    bool m_running = false;
};

} // namespace rpc
} // namespace sc

#endif