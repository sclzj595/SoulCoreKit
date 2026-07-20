/**
 * @file core/network_state.h
 * @brief 网络状态枚举
 * @details 定义网络连接状态的枚举类型
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_CORE_NETWORK_STATE_H
#define SOUL_NETWORK_CORE_NETWORK_STATE_H

namespace sc {
namespace network {

enum class NetworkState {
    Created,
    Connecting,
    Connected,
    Working,
    Reconnecting,
    Disconnected,
    Destroyed
};

} // namespace network
} // namespace sc

#endif
