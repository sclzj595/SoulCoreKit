/**
 * @file soul_network.h
 * @brief 网络模块统一入口头文件
 * @details 包含网络模块所有核心类的引用，提供便捷的单文件包含方式
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */

#ifndef SOUL_NETWORK_H
#define SOUL_NETWORK_H

#include "soul/network/network_global.h"

#include "soul/network/http_request.h"
#include "soul/network/http_response.h"
#include "soul/network/http_client.h"
#include "soul/network/web_socket.h"
#include "soul/network/tcp_client.h"
#include "soul/network/session.h"
#include "soul/network/downloader.h"
#include "soul/network/uploader.h"
#include "soul/network/network_error.h"
#include "soul/network/cookie_jar.h"

#include "soul/network/core/inetwork.h"
#include "soul/network/core/network_base.h"
#include "soul/network/core/network_adapter_base.h"
#include "soul/network/core/network_message.h"
#include "soul/network/core/network_state.h"

#include "soul/network/policy/inetwork_policy.h"
#include "soul/network/policy/retry_policy.h"
#include "soul/network/policy/timeout_policy.h"
#include "soul/network/policy/reconnect_policy.h"
#include "soul/network/policy/heartbeat_policy.h"

#include "soul/network/interceptor/i_interceptor.h"
#include "soul/network/interceptor/logging_interceptor.h"
#include "soul/network/interceptor/auth_interceptor.h"
#include "soul/network/http/http_interceptor.h"

#include "soul/network/codec/icodec.h"
#include "soul/network/codec/json_codec.h"
#include "soul/network/codec/codec_factory.h"

#include "soul/network/monitor/metrics.h"
#include "soul/network/monitor/monitor.h"

#include "soul/network/pool/connection_pool.h"

#include "soul/network/factory/network_factory.h"

#include "soul/network/http/http_client_adapter.h"
#include "soul/network/tcp/tcp_client_adapter.h"
#include "soul/network/websocket/ws_client_adapter.h"
#include "soul/network/mqtt/mqtt_client_adapter.h"
#include "soul/network/bluetooth/bluetooth_client_adapter.h"
#include "soul/network/serial/serial_port_adapter.h"
#include "soul/network/namedpipe/named_pipe_adapter.h"

#endif