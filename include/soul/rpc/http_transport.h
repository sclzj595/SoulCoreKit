#ifndef SOUL_RPC_HTTP_TRANSPORT_H
#define SOUL_RPC_HTTP_TRANSPORT_H

#include "soul/rpc/irpc_transport.h"
#include "soul/rpc/iserializer.h"
#include <QObject>
#include <QNetworkAccessManager>
#include <QMap>
#include <atomic>
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

    void setReadTimeout(int ms);

    /**
     * @brief 启用或禁用 HTTP/2 多路复用(v1.8.0 新增)
     * @param enabled true 启用(默认),false 禁用
     * @details 默认启用 HTTP/2。服务器不支持时 Qt 自动降级到 HTTP/1.1。
     */
    void setHttp2Enabled(bool enabled);

    /**
     * @brief 查询 HTTP/2 是否启用
     * @return true 启用,false 禁用
     */
    bool isHttp2Enabled() const;

private:
    QString m_baseUrl;
    QMap<QString, QString> m_headers;
    QNetworkAccessManager* m_manager;
    std::shared_ptr<ISerializer> m_serializer;
    int m_readTimeout = 30000;
    // TSan-safe: m_running / m_http2Enabled may be set by other threads via
    // start()/stop()/setHttp2Enabled() while sendRequest() reads them.
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_http2Enabled{true};  ///< HTTP/2 默认启用(v1.8.0)
};

} // namespace rpc
} // namespace sc

#endif