#ifndef SOUL_SERVER_SHUTDOWN_ENDPOINT_H
#define SOUL_SERVER_SHUTDOWN_ENDPOINT_H

// ============================================================================
// shutdown_endpoint.h — 优雅停机端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/shutdown (POST),触发应用优雅停机。
// 本端点仅返回停机响应消息,实际的停机动作由 HttpServer::shutdown()
// 在 POST 路由处理中调用。
//
// 用法:
//   server.post("/actuator/shutdown", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(ShutdownEndpoint::toJson());
//       server.shutdown();  // 实际停机动作
//   });

#include "soul/utils/json/json_helper.h"

#include <QByteArray>

namespace sc {
namespace server {

// ============================================================================
// ShutdownEndpoint — 优雅停机端点
// ============================================================================
class ShutdownEndpoint {
public:
    /// @brief 返回停机响应 JSON
    /// @return JSON: {"message": "Shutting down, bye..."}
    /// @note 实际的停机动作由 HttpServer::shutdown() 在路由处理中调用
    static QByteArray toJson() {
        sc::json::Json root = sc::json::Json::object();
        root["message"] = "Shutting down, bye...";
        return sc::json::serializePretty(root);
    }
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_SHUTDOWN_ENDPOINT_H
