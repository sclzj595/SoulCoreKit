/**
 * @file core/network_message.h
 * @brief 网络消息类
 * @details 统一的消息封装，包含 API 路径、负载和元数据
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CORE_NETWORK_MESSAGE_H
#define SOUL_NETWORK_CORE_NETWORK_MESSAGE_H

#include <memory>
#include <QString>
#include <QByteArray>
#include <QVariantMap>
#include "soul/network/network_error.h"

namespace sc {
namespace network {

struct NetworkMessage {
    QString api;
    QByteArray payload;
    QVariantMap metadata;
    int statusCode = 0;
    QString message;
    qint64 duration = 0;
    std::shared_ptr<NetworkError> error;
};

} // namespace network
} // namespace sc

#endif
