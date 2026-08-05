// ============================================================================
// mappings_endpoint.cpp — 路由映射端点实现 [v1.9.4]
// ============================================================================

#include "soul/server/mappings_endpoint.h"
#include "soul/server/http_server.h"

namespace sc {
namespace server {

QByteArray MappingsEndpoint::toJson(const HttpServer& server) {
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        if (s_useCustomMappings) {
            sc::json::Json root = sc::json::Json::array();
            for (const auto& m : s_customMappings) {
                sc::json::Json entry = sc::json::Json::object();
                entry["method"] = m.method;
                entry["path"] = m.path;
                root.push_back(entry);
            }
            return sc::json::serializePretty(root);
        }
    }

    // 锁已释放,安全调用 server.getRoutes() — HttpServer 有自己的锁
    auto routes = server.getRoutes();
    sc::json::Json root = sc::json::Json::array();
    for (const auto& m : routes) {
        sc::json::Json entry = sc::json::Json::object();
        entry["method"] = m.method;
        entry["path"] = m.path;
        root.push_back(entry);
    }
    return sc::json::serializePretty(root);
}

} // namespace server
} // namespace sc