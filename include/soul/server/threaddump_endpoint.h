#ifndef SOUL_SERVER_THREADDUMP_ENDPOINT_H
#define SOUL_SERVER_THREADDUMP_ENDPOINT_H

// ============================================================================
// threaddump_endpoint.h — 线程转储端点 [v1.9.4]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/threaddump,返回当前线程及线程池信息。
// 受限于跨平台线程枚举能力,本端点仅稳定暴露当前线程的快照信息,
// 同时附带硬件并发数与全局线程池最大线程数,供运维监控使用。
//
// 用法:
//   server.get("/actuator/threaddump", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(ThreadDumpEndpoint::toJson());
//   });

#include "soul/utils/json/json_helper.h"

#include <QByteArray>
#include <QString>
#include <QThread>
#include <QThreadPool>
#include <thread>

namespace sc {
namespace server {

// ============================================================================
// ThreadDumpEndpoint — 线程转储端点
// ============================================================================
class ThreadDumpEndpoint {
public:
    /// @brief 序列化为 JSON (使用 nlohmann::json)
    /// @return JSON 结构:
    ///   {
    ///     "threads": [{"id":..., "name":"...", "priority":..., "isCurrent":...}],
    ///     "hardwareConcurrency": N,
    ///     "poolMaxThreadCount": N
    ///   }
    static QByteArray toJson() {
        sc::json::Json root = sc::json::Json::object();

        // 当前线程信息(跨平台只能稳定获取当前线程)
        sc::json::Json threads = sc::json::Json::array();
        {
            QThread* cur = QThread::currentThread();
            sc::json::Json t = sc::json::Json::object();
            // Qt::HANDLE (void*) 转为无符号整数,作为线程唯一标识
            t["id"] = reinterpret_cast<quintptr>(QThread::currentThreadId());

            std::string name = cur->objectName().toStdString();
            if (name.empty()) {
                name = "main";
            }
            t["name"]      = name;
            t["priority"]  = static_cast<int>(cur->priority());
            t["isCurrent"] = true;
            threads.push_back(t);
        }
        root["threads"] = threads;

        // 硬件并发数 (CPU 逻辑核心数)
        root["hardwareConcurrency"] =
            static_cast<unsigned int>(std::thread::hardware_concurrency());

        // 全局线程池最大线程数
        root["poolMaxThreadCount"] = QThreadPool::globalInstance()->maxThreadCount();

        return sc::json::serializePretty(root);
    }
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_THREADDUMP_ENDPOINT_H
