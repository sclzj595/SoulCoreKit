/**
 * @file interceptor/auth_interceptor.h
 * @brief 认证拦截器
 * @details 自动添加认证 Token 到请求头
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_INTERCEPTOR_AUTH_INTERCEPTOR_H
#define SOUL_NETWORK_INTERCEPTOR_AUTH_INTERCEPTOR_H

#include "soul/network/interceptor/i_interceptor.h"
#include <QString>

namespace sc {
namespace network {

class SC_NETWORK_EXPORT AuthInterceptor : public IInterceptor {
public:
    AuthInterceptor();
    explicit AuthInterceptor(const QString& token);

    ~AuthInterceptor() override = default;

    void onRequest(NetworkMessage& request) override;
    void onResponse(NetworkMessage& response) override;

    void setToken(const QString& token);
    QString token() const;

    std::string interfaceName() const override {
        return "AuthInterceptor";
    }

private:
    QString m_token;
};

} // namespace network
} // namespace sc

#endif