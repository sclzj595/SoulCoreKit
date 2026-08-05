#ifndef SOUL_SERVER_ENV_ENDPOINT_H
#define SOUL_SERVER_ENV_ENDPOINT_H

// ============================================================================
// env_endpoint.h — 环境配置端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/env,暴露应用运行环境信息
// (配置属性/Profile/环境变量/系统属性)。
//
// 用法:
//   server.get("/actuator/env", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(EnvironmentEndpoint::toJson());
//   });

#include "soul/utils/json/json_helper.h"

#include <QByteArray>
#include <QProcessEnvironment>
#include <QString>
#include <cstdlib>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace sc {
namespace server {

// ============================================================================
// EnvironmentEndpoint — 环境配置端点
// ============================================================================
class EnvironmentEndpoint {
public:
    /// @brief 设置活跃的 Profile 列表
    /// @thread_safety 线程安全
    static void setActiveProfiles(const std::vector<std::string>& profiles) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_activeProfiles = profiles;
    }

    /// @brief 添加自定义属性
    /// @thread_safety 线程安全
    static void setProperty(const std::string& key, const std::string& value) {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_customProperties[key] = value;
    }

    /// @brief 清空所有自定义属性(用于测试清理)
    /// @thread_safety 线程安全
    static void clearProperties() {
        std::lock_guard<std::mutex> lock(s_mutex);
        s_customProperties.clear();
    }

    /// @brief 序列化为 JSON
    /// @thread_safety 线程安全
    static QByteArray toJson() {
        std::lock_guard<std::mutex> lock(s_mutex);
        sc::json::Json root = sc::json::Json::object();

        // activeProfiles
        sc::json::Json profiles = sc::json::Json::array();
        for (const auto& p : s_activeProfiles) {
            profiles.push_back(p);
        }
        root["activeProfiles"] = profiles;

        // propertySources
        sc::json::Json sources = sc::json::Json::array();

        // 1. 自定义属性
        {
            sc::json::Json src = sc::json::Json::object();
            src["name"] = "customProperties";
            sc::json::Json props = sc::json::Json::object();
            for (const auto& [k, v] : s_customProperties) {
                props[k] = {{"value", v}};
            }
            src["properties"] = props;
            sources.push_back(src);
        }

        // 2. 系统环境变量
        {
            sc::json::Json src = sc::json::Json::object();
            src["name"] = "systemEnvironment";
            sc::json::Json props = sc::json::Json::object();
#if defined(Q_OS_WIN)
            // Windows: 使用 GetEnvironmentVariableW 避免 Qt 编码问题
            const auto& env = QProcessEnvironment::systemEnvironment();
            const QStringList keys = env.keys();
            for (const auto& key : keys) {
                props[key.toStdString()] = {{"value", env.value(key).toStdString()}};
            }
#else
            // Linux/macOS: environ 原生访问
            extern char** environ;
            if (environ) {
                for (char** envp = environ; *envp != nullptr; ++envp) {
                    std::string entry(*envp);
                    auto eqPos = entry.find('=');
                    if (eqPos != std::string::npos) {
                        props[entry.substr(0, eqPos)] = {
                            {"value", entry.substr(eqPos + 1)}
                        };
                    }
                }
            }
#endif
            src["properties"] = props;
            sources.push_back(src);
        }

        root["propertySources"] = sources;
        return sc::json::serializePretty(root);
    }

private:
    inline static std::mutex s_mutex;
    inline static std::vector<std::string> s_activeProfiles = {"default"};
    inline static std::map<std::string, std::string> s_customProperties;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_ENV_ENDPOINT_H