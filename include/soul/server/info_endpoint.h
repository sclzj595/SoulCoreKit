#ifndef SOUL_SERVER_INFO_ENDPOINT_H
#define SOUL_SERVER_INFO_ENDPOINT_H

// ============================================================================
// info_endpoint.h — 应用信息端点 [v1.9.3]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/info,返回应用元数据(版本号/构建信息等)。
//
// 用法:
//   server.get("/actuator/info", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(InfoEndpoint::toJson());
//   });

#include "soul/utils/json/json_helper.h"

#include <QByteArray>
#include <QString>
#include <chrono>
#include <string>

namespace sc {
namespace server {

// ============================================================================
// InfoEndpoint — 应用信息端点
// ============================================================================
class InfoEndpoint {
public:
    /// @brief 设置应用名称
    static void setAppName(const std::string& name) { s_appName = name; }
    static std::string appName() { return s_appName; }

    /// @brief 设置应用版本
    static void setAppVersion(const std::string& version) { s_appVersion = version; }
    static std::string appVersion() { return s_appVersion; }

    /// @brief 设置应用描述
    static void setDescription(const std::string& desc) { s_description = desc; }
    static std::string description() { return s_description; }

    /// @brief 设置启动时间(epoch milliseconds)
    static void setStartupTime(int64_t ms) { s_startupTimeMs = ms; }
    static int64_t startupTimeMs() { return s_startupTimeMs; }

    /// @brief 序列化为 JSON (使用 nlohmann::json,与项目统一 JSON 方案一致)
    static QByteArray toJson() {
        sc::json::Json root = sc::json::Json::object();
        sc::json::Json app  = sc::json::Json::object();
        app["name"] = s_appName;
        app["version"] = s_appVersion;
        if (!s_description.empty()) {
            app["description"] = s_description;
        }
        app["startupTimeMs"] = s_startupTimeMs;
        root["app"] = app;

        sc::json::Json framework = sc::json::Json::object();
        framework["name"] = "SoulCoreKit";
        framework["version"] = s_frameworkVersion;
        root["framework"] = framework;

        return sc::json::serializePretty(root);
    }

    /// @brief 设置框架版本(由 Scaffold 自动调用)
    static void setFrameworkVersion(const std::string& version) {
        s_frameworkVersion = version;
    }

private:
    inline static std::string s_appName = "SoulCoreKit App";
    inline static std::string s_appVersion = "1.0.0";
    inline static std::string s_description;
    inline static std::string s_frameworkVersion = "1.9.3";
    inline static int64_t s_startupTimeMs = 0;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_INFO_ENDPOINT_H