#ifndef SOUL_NETWORK_CORE_CONNECTION_STATE_MACHINE_H
#define SOUL_NETWORK_CORE_CONNECTION_STATE_MACHINE_H

#include <mutex>
#include <functional>
#include <string>

namespace sc {
namespace network {

enum class ConnectionState {
    Disconnected,
    Connecting,
    Connected,
    Disconnecting,
    Error
};

class ConnectionStateMachine {
public:
    using StateChangedCallback = std::function<void(ConnectionState oldState, ConnectionState newState)>;

    ConnectionStateMachine() : m_state(ConnectionState::Disconnected) {}

    ConnectionState state() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_state;
    }

    bool isConnected() const {
        return state() == ConnectionState::Connected;
    }

    bool isDisconnected() const {
        return state() == ConnectionState::Disconnected;
    }

    bool isConnecting() const {
        return state() == ConnectionState::Connecting;
    }

    bool isDisconnecting() const {
        return state() == ConnectionState::Disconnecting;
    }

    bool isError() const {
        return state() == ConnectionState::Error;
    }

    bool canConnect() const {
        ConnectionState s = state();
        return s == ConnectionState::Disconnected || s == ConnectionState::Error;
    }

    bool canDisconnect() const {
        ConnectionState s = state();
        return s == ConnectionState::Connected || s == ConnectionState::Connecting;
    }

    bool canSend() const {
        return state() == ConnectionState::Connected;
    }

    bool transitionToConnecting() {
        return transition(ConnectionState::Disconnected, ConnectionState::Connecting) ||
               transition(ConnectionState::Error, ConnectionState::Connecting);
    }

    bool transitionToConnected() {
        return transition(ConnectionState::Connecting, ConnectionState::Connected);
    }

    bool transitionToDisconnecting() {
        return transition(ConnectionState::Connected, ConnectionState::Disconnecting) ||
               transition(ConnectionState::Connecting, ConnectionState::Disconnecting);
    }

    bool transitionToDisconnected() {
        return transition(ConnectionState::Connecting, ConnectionState::Disconnected) ||
               transition(ConnectionState::Connected, ConnectionState::Disconnected) ||
               transition(ConnectionState::Disconnecting, ConnectionState::Disconnected);
    }

    bool transitionToError() {
        return transition(ConnectionState::Connecting, ConnectionState::Error) ||
               transition(ConnectionState::Connected, ConnectionState::Error);
    }

    void setStateChangedCallback(StateChangedCallback callback) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_callback = callback;
    }

    std::string stateToString(ConnectionState state) const {
        switch (state) {
        case ConnectionState::Disconnected: return "Disconnected";
        case ConnectionState::Connecting: return "Connecting";
        case ConnectionState::Connected: return "Connected";
        case ConnectionState::Disconnecting: return "Disconnecting";
        case ConnectionState::Error: return "Error";
        default: return "Unknown";
        }
    }

private:
    bool transition(ConnectionState expected, ConnectionState next) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_state != expected) {
            return false;
        }
        ConnectionState oldState = m_state;
        m_state = next;
        if (m_callback) {
            m_callback(oldState, m_state);
        }
        return true;
    }

    mutable std::mutex m_mutex;
    ConnectionState m_state;
    StateChangedCallback m_callback;
};

} // namespace network
} // namespace sc

#endif