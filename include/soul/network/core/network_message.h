#ifndef SOUL_NETWORK_CORE_NETWORK_MESSAGE_H
#define SOUL_NETWORK_CORE_NETWORK_MESSAGE_H

#include <memory>
#include <QString>
#include <QByteArray>
#include <QVariantMap>
#include "soul/network/network_error.h"

namespace sc::network {

struct NetworkMessage {
    QString api;
    QByteArray payload;
    QVariantMap metadata;
    int statusCode = 0;
    QString message;
    qint64 duration = 0;
    std::shared_ptr<NetworkError> error;
};

}

#endif
