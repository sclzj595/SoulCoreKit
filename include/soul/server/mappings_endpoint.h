#ifndef SOUL_SERVER_MAPPINGS_ENDPOINT_H
#define SOUL_SERVER_MAPPINGS_ENDPOINT_H

// ============================================================================
// mappings_endpoint.h — 路由映射端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/mappings,暴露所有已注册的路由映射。
//
// 用法:
//   server.get("/actuator/mappings", [&server](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(MappingsEndpoint::toJson(server));
//   });

#include "soul/utils/json/json_helper.h"

#include <QByteArray>
#include <QString>
#include <mutex>
#include <string>
#include <vector>

namespace sc {
namespace server {

// 前置声明
class HttpServer;

// ============================================================================
// RouteMapping — 单条路由映射信息
// ============================================================================
struct RouteMapping {
    std::string method;  ///< HTTP 方法(GET/POST/PUT/DELETE/HEAD/OPTIONS/PATCH)
    std::string path;    ///< 路由路径
};

// ============================================================================
// MappingsEndpoint — 路由映射端点
// ============================================================================
class MappingsEndpoint {
public:
    /// @brief 从 HttpServer 的路由表导出所有映射为 JSON
    /// @param server HttpServer 实例(用于获取路由表)
    /// @return JSON 格式的路由映射列表
    static QByteArray toJson(const HttpServer& server);

    /// @brief 手动设置路由映射列表(用于测试或自定义路由)
    /// @thread_safety 线程安全
    static void setMappings(const std::vector<RouteMapping>& mappings) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_customMappings = mappings;
        s_useCustomMappings = true;
    }

    /// @brief 重置为从 HttpServer 获取路由
    /// @thread_safety 线程安全
    static void resetToServerSource() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_useCustomMappings = false;
        s_customMappings.clear();
    }

private:
    inline static std::mutex s_mutex;
    inline static bool s_useCustomMappings = false;
    inline static std::vector<RouteMapping> s_customMappings;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_MAPPINGS_ENDPOINT_H