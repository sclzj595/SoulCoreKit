#ifndef SOUL_NETWORK_AGGREGATE_H
#define SOUL_NETWORK_AGGREGATE_H

// soul_network.h — Network 模块聚合头文件
//
// 一行引入网络通信层公共 API:HttpClient/HttpRequest/HttpResponse/HttpApi/
// WebSocket/TcpClient/Session/Downloader/Uploader/NetworkFactory 及策略/拦截器。
//
// 用法:
//   #include "soul/soul_network.h"
//
// 注意: 各协议适配器(HTTP/WebSocket/TCP/MQTT/Bluetooth/Serial/NamedPipe)位于
// 各子头文件中,按需引入,例如 #include "soul/network/http/http_client_adapter.h"。

#include "soul/network/network_global.h"
#include "soul/network/http_request.h"
#include "soul/network/http_response.h"
#include "soul/network/http_client.h"
#include "soul/network/http_api.h"
#include "soul/network/web_socket.h"
#include "soul/network/tcp_client.h"
#include "soul/network/session.h"
#include "soul/network/downloader.h"
#include "soul/network/uploader.h"
#include "soul/network/network_error.h"
#include "soul/network/cookie_jar.h"
#include "soul/network/factory/network_factory.h"
#include "soul/network/policy/inetwork_policy.h"
#include "soul/network/policy/timeout_policy.h"
#include "soul/network/policy/retry_policy.h"
#include "soul/network/policy/reconnect_policy.h"
#include "soul/network/policy/heartbeat_policy.h"
#include "soul/network/interceptor/i_interceptor.h"
#include "soul/network/interceptor/logging_interceptor.h"
#include "soul/network/interceptor/auth_interceptor.h"

#endif // SOUL_NETWORK_AGGREGATE_H
