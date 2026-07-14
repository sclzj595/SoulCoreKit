#ifndef SOUL_NETWORK_INTERCEPTOR_AUTH_INTERCEPTOR_H
#define SOUL_NETWORK_INTERCEPTOR_AUTH_INTERCEPTOR_H

#include "soul/network/interceptor/i_interceptor.h"
#include <QString>

namespace sc::network {

class AuthInterceptor : public IInterceptor {
public:
    AuthInterceptor();
    explicit AuthInterceptor(const QString& token);

    ~AuthInterceptor() override = default;

    void onRequest(NetworkMessage& request) override;
    void onResponse(NetworkMessage& response) override;

    void setToken(const QString& token);
    QString token() const;

    std::string interfaceName() const override {
        return "sc::network::AuthInterceptor";
    }

private:
    QString m_token;
};

}

#endif