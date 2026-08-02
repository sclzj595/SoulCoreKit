#ifndef SOUL_SERVER_LOGGERS_ENDPOINT_H
#define SOUL_SERVER_LOGGERS_ENDPOINT_H

// ============================================================================
// loggers_endpoint.h — 日志级别管理端点 [v1.9.3]
// ============================================================================
//
// 对标 SpringBoot Actuator /actuator/loggers,支持运行时查看和动态调整日志级别。
//
// 用法:
//   // GET /actuator/loggers — 查看所有日志级别
//   server.get("/actuator/loggers", [](const HttpRequest&, HttpResponse& resp) {
//       resp.setHeader("Content-Type", "application/json");
//       resp.setBody(LoggersEndpoint::getAllLevels());
//   });
//
//   // POST /actuator/loggers/{name} — 设置指定 logger 级别
//   // body: {"configuredLevel": "DEBUG"}
//   server.post("/actuator/loggers/soulcore", [](const HttpRequest& req, HttpResponse& resp) {
//       auto json = sc::json::parse(req.body());
//       std::string level = json["configuredLevel"].get<std::string>();
//       LoggersEndpoint::setLevel("soulcore", level);
//       resp.setStatus(200);
//   });

#include "soul/utils/json/json_helper.h"
#include "soul/logging/logger.h"
#include "soul/logging/log_level.h"

#include <QByteArray>
#include <QString>
#include <map>
#include <mutex>
#include <string>

namespace sc {
namespace server {

// ============================================================================
// LoggersEndpoint — 日志级别管理端点
// ============================================================================
//
// @thread_safety 线程安全 — 所有方法持 m_mutex 保护
class LoggersEndpoint {
public:
    /// @brief 获取所有已配置的日志级别(JSON,使用 nlohmann::json)
    static QByteArray getAllLevels() {
        std::lock_guard<std::mutex> lock(s_mutex);

        sc::json::Json root = sc::json::Json::object();

        // 有效级别列表
        root["levels"] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

        // loggers
        sc::json::Json loggers = sc::json::Json::object();
        loggers["ROOT"] = {{"configuredLevel", levelToString(Logger::instance().logLevel())}};

        for (const auto& [name, level] : s_moduleLevels) {
            loggers[name] = {{"configuredLevel", levelToString(level)}};
        }
        root["loggers"] = loggers;

        return sc::json::serializePretty(root);
    }

    /// @brief 设置指定 logger 的日志级别
    /// @param name  logger 名称("ROOT" 表示根 logger)
    /// @param level 日志级别字符串(TRACE/DEBUG/INFO/WARN/ERROR/FATAL)
    /// @return true 成功, false 无效级别
    static bool setLevel(const std::string& name, const std::string& level) {
        LogLevel lvl = levelFromString(level);
        // levelFromString 默认返回 Info,需区分 "INFO" 和无效输入
        if (lvl == LogLevel::Info && level != "INFO") return false;

        std::lock_guard<std::mutex> lock(s_mutex);

        if (name == "ROOT") {
            Logger::instance().setLogLevel(lvl);
        } else {
            Logger::instance().setModuleLogLevel(name, lvl);
            s_moduleLevels[name] = lvl;
        }
        return true;
    }

    /// @brief 获取指定 logger 的当前级别
    static std::string getLevel(const std::string& name) {
        if (name == "ROOT") {
            return levelToString(Logger::instance().logLevel());
        }
        return levelToString(Logger::instance().moduleLogLevel(name));
    }

private:
    static std::string levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace: return "TRACE";
            case LogLevel::Debug: return "DEBUG";
            case LogLevel::Info:  return "INFO";
            case LogLevel::Warn:  return "WARN";
            case LogLevel::Error: return "ERROR";
            case LogLevel::Fatal: return "FATAL";
        }
        return "INFO";
    }

    static LogLevel levelFromString(const std::string& level) {
        // 大小写不敏感匹配,对标 SpringBoot Actuator
        std::string upper = level;
        for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (upper == "TRACE") return LogLevel::Trace;
        if (upper == "DEBUG") return LogLevel::Debug;
        if (upper == "INFO")  return LogLevel::Info;
        if (upper == "WARN")  return LogLevel::Warn;
        if (upper == "ERROR") return LogLevel::Error;
        if (upper == "FATAL") return LogLevel::Fatal;
        return LogLevel::Info;  // 无效输入使用默认值,由 setLevel() 的二次校验拒绝
    }

    inline static std::mutex s_mutex;
    inline static std::map<std::string, LogLevel> s_moduleLevels;
};

} // namespace server
} // namespace sc

#endif // SOUL_SERVER_LOGGERS_ENDPOINT_H