#ifndef SOUL_NETWORK_POLICY_RECONNECT_POLICY_H
#define SOUL_NETWORK_POLICY_RECONNECT_POLICY_H

#include <chrono>

namespace sc::network {

struct ReconnectPolicy {
    bool enabled = false;
    std::chrono::milliseconds interval = std::chrono::seconds(5);
    int maxRetries = 0;
    int currentRetry = 0;

    bool shouldReconnect() const {
        if (!enabled) return false;
        if (maxRetries <= 0) return true;
        return currentRetry < maxRetries;
    }

    void resetRetry() {
        currentRetry = 0;
    }

    void incrementRetry() {
        currentRetry++;
    }
};

}

#endif