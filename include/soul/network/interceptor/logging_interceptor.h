/**
 * @file interceptor/logging_interceptor.h
 * @brief 日志拦截器
 * @details 记录网络请求和响应的日志信息
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_INTERCEPTOR_LOGGING_INTERCEPTOR_H
#define SOUL_NETWORK_INTERCEPTOR_LOGGING_INTERCEPTOR_H

#include "soul/network/network_global.h"
#include "soul/network/interceptor/i_interceptor.h"
#include "soul/network/core/network_message.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT LoggingInterceptor : public IInterceptor<NetworkMessage, NetworkMessage> {
public:
    ~LoggingInterceptor() override = default;

    void onRequest(NetworkMessage& request) override;
    void onResponse(NetworkMessage& response) override;

    std::string interfaceName() const override {
        return "LoggingInterceptor";
    }
};

} // namespace network
} // namespace sc

#endif