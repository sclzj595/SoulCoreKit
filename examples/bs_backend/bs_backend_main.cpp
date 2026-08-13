// ============================================================================
// bs_backend_main.cpp — BS Web Backend 端到端示例 [v3.0.0 适配]
// ============================================================================
//
// 演示 SoulCoreKit 作为 BS 后端的完整能力:
//   - 嵌入式 HTTP Server (HttpServer)
//   - 中间件链 (Logging → Trace → Auth → RateLimit → Validation)
//   - RESTful 路由 (路径参数 :id)
//   - 健康检查 (/health, /ready)
//   - Scaffold 生命周期管理

#include "soul/soul.h"
#include "soul/server/http_server.h"
#include "soul/server/middleware.h"
#include "soul/server/mappings_endpoint.h"
#include "soul/core/health.h"          // v2.8.0
#include "soul/core/request_context.h" // v2.8.0
#include "soul/logging/log_macros.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

using namespace sc;
using namespace sc::server;

// ============================================================================
// 模拟用户数据
// ============================================================================

struct User {
    int id;
    QString name;
    QString email;
};

static std::vector<User> g_users = {
    {1, "Alice", "alice@example.com"},
    {2, "Bob", "bob@example.com"},
    {42, "SoulCoreKit", "soul@example.com"},
};

// ============================================================================
// BS 后端模块
// ============================================================================

class BsBackendModule : public Module {
public:
    BsBackendModule() : Module("BsBackend") {}

    Result<void> init() override {
        // HttpServer 不是单例，由模块持有生命周期
        m_server = std::make_unique<HttpServer>();

        // --- 中间件链 ---
        m_server->use(std::make_shared<LoggingMiddleware>());

        auto trace = std::make_shared<TraceMiddleware>();
        trace->setHeaderPrefix("X-");
        m_server->use(trace);

        // AuthMiddleware: 简单 Token 校验 (demo 用)
        auto auth = std::make_shared<AuthMiddleware>(
            [](const std::string& token) { return token == "demo-token"; }
        );
        auth->addExcludePath("/api/health");
        auth->addExcludePath("/api/ready");
        auth->addExcludePath("/api/live");
        m_server->use(auth);

        auto validation = std::make_shared<ValidationMiddleware>();
        validation->addExcludePath("/api/health");
        validation->addExcludePath("/api/ready");
        validation->addExcludePath("/api/live");
        validation->setMaxBodySize(1024 * 1024);  // 1MB
        m_server->use(validation);

        // --- 健康检查 ---
        auto& health = HealthAggregator::instance();
        health.registerIndicator(std::make_shared<LivenessIndicator>());
        health.registerIndicator(std::make_shared<MemoryHealthIndicator>(2048));  // 2GB 阈值

        // --- 路由注册 ---
        registerRoutes(*m_server);

        // --- 启动 ---
        m_server->listen(QHostAddress::Any, 8080);
        SC_INFO("BS Backend listening on http://0.0.0.0:8080");
        return {};
    }

    Result<void> onStart() override {
        SC_INFO("BS Backend started successfully");
        return {};
    }

    void onStop() override {
        SC_INFO("BS Backend stopping...");
    }

    void cleanup() override {
        if (m_server) {
            m_server->close();
        }
        SC_INFO("BS Backend shut down");
    }

private:
    std::unique_ptr<HttpServer> m_server;

    void registerRoutes(HttpServer& server) {
        // --- 标准健康检查端点 ---

        server.get("/api/live", [](const HttpRequest&, HttpResponse& resp) {
            resp.setStatus(200);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(R"({"status":"UP"})");
        });

        server.get("/api/ready", [](const HttpRequest&, HttpResponse& resp) {
            auto& health = HealthAggregator::instance();
            resp.setStatus(health.isReady() ? 200 : 503);
            resp.setHeader("Content-Type", "application/json");
            if (health.isReady()) {
                resp.setBody(R"({"ready":true})");
            } else {
                resp.setBody(R"({"ready":false,"reason":"Dependencies unhealthy"})");
            }
        });

        server.get("/api/health", [](const HttpRequest&, HttpResponse& resp) {
            auto& health = HealthAggregator::instance();
            auto json = health.toJson();

            auto status = health.isHealthy() ? 200 :
                         health.isReady() ? 200 : 503;
            resp.setStatus(status);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(json);
        });

        // --- RESTful API: 用户管理 ---

        server.get("/api/users", [](const HttpRequest&, HttpResponse& resp) {
            QJsonArray arr;
            for (const auto& u : g_users) {
                QJsonObject obj;
                obj["id"] = u.id;
                obj["name"] = u.name;
                obj["email"] = u.email;
                arr.append(obj);
            }
            QJsonDocument doc(arr);
            resp.setStatus(200);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(doc.toJson(QJsonDocument::Compact));
        });

        server.get("/api/users/:id", [](const HttpRequest& req, HttpResponse& resp) {
            QString idStr = req.pathParam("id");
            if (idStr.isEmpty()) {
                resp.setStatus(400);
                resp.setBody(R"({"error":"Missing user id"})");
                return;
            }

            int id = idStr.toInt();
            auto it = std::find_if(g_users.begin(), g_users.end(),
                [id](const User& u) { return u.id == id; });

            if (it == g_users.end()) {
                resp.setStatus(404);
                resp.setBody(R"({"error":"User not found"})");
                return;
            }

            QJsonObject obj;
            obj["id"] = it->id;
            obj["name"] = it->name;
            obj["email"] = it->email;

            QJsonDocument doc(obj);
            resp.setStatus(200);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(doc.toJson(QJsonDocument::Compact));
        });

        server.post("/api/users", [](const HttpRequest& req, HttpResponse& resp) {
            QJsonDocument doc = QJsonDocument::fromJson(req.body());
            if (!doc.isObject()) {
                resp.setStatus(400);
                resp.setBody(R"({"error":"Invalid JSON"})");
                return;
            }

            QJsonObject obj = doc.object();
            User newUser;
            newUser.id = g_users.empty() ? 1 : g_users.back().id + 1;
            newUser.name = obj["name"].toString();
            newUser.email = obj["email"].toString();
            g_users.push_back(newUser);

            QJsonObject result;
            result["id"] = newUser.id;
            result["name"] = newUser.name;
            result["email"] = newUser.email;

            QJsonDocument resultDoc(result);
            resp.setStatus(201);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(resultDoc.toJson(QJsonDocument::Compact));
        });

        // 404 处理
        server.setNotFoundHandler([](const HttpRequest&, HttpResponse& resp) {
            resp.setStatus(404);
            resp.setBody(R"({"error":"Not Found"})");
        });

        // 暴露路由列表 (调试用)
        server.get("/api/routes", [&server](const HttpRequest&, HttpResponse& resp) {
            auto routes = server.getRoutes();
            QJsonArray arr;
            for (const auto& r : routes) {
                QJsonObject obj;
                obj["method"] = QString::fromStdString(r.method);
                obj["path"] = QString::fromStdString(r.path);
                arr.append(obj);
            }
            QJsonDocument doc(arr);
            resp.setStatus(200);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(doc.toJson(QJsonDocument::Compact));
        });
    }
};

// ============================================================================
// main
// ============================================================================

int main(int argc, char* argv[]) {
    BsBackendModule backend;
    Scaffold scaffold(argc, argv);
    scaffold.use(backend);
    return scaffold.run();
}
