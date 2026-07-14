#ifndef SOUL_NETWORK_CORE_NETWORK_STATE_H
#define SOUL_NETWORK_CORE_NETWORK_STATE_H

namespace sc::network {

enum class NetworkState {
    Created,
    Connecting,
    Connected,
    Working,
    Reconnecting,
    Disconnected,
    Destroyed
};

}

#endif
