#ifndef SOUL_NETWORK_CORE_NETWORK_BASE_H
#define SOUL_NETWORK_CORE_NETWORK_BASE_H

#include <QObject>
#include "soul/network/core/network_message.h"
#include "soul/network/network_error.h"

namespace sc::network {

class NetworkBase : public QObject {
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

}

#endif
