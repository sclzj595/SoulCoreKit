/**
 * @file core/network_base.h
 * @brief 网络基类
 * @details 网络模块的抽象基类，提供公共功能和生命周期管理
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CORE_NETWORK_BASE_H
#define SOUL_NETWORK_CORE_NETWORK_BASE_H

#include <QObject>
#include "soul/network/network_global.h"
#include "soul/network/core/network_message.h"
#include "soul/network/network_error.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT NetworkBase : public QObject {
    Q_OBJECT
public:
    explicit NetworkBase(QObject* parent = nullptr) : QObject(parent) {}
signals:
    void connected();
    void disconnected();
    void received(const NetworkMessage& msg);
    void error(const NetworkError& error);
    void progress(qint64 current, qint64 total);
    void timeout();
    void reconnect(int attempt);
};

} // namespace network
} // namespace sc

#endif
