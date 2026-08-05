// ============================================================================
// full_stack_example.cpp — SoulCoreKit v1.9.4 全栈集成示例
// ============================================================================
//
// 演示如何将一个 CS 架构 Server 端从零搭建为生产可用的 SpringBoot 式脚手架:
//   - Scaffold 模块生命周期管理
//   - HttpServer 路由 + 中间件链(日志/鉴权/CORS/限流)
//   - HealthEndpoint 健康检查(liveness/readiness)
//   - PrometheusExporter 指标导出
//   - InfoEndpoint 应用信息
//   - LoggersEndpoint 动态日志级别
//   - EnvironmentEndpoint 环境配置 [v1.9.4]
//   - MappingsEndpoint 路由映射 [v1.9.4]
//   - MetricsEndpoint 指标查询 [v1.9.4]
//   - ThreadDumpEndpoint 线程转储 [v1.9.4]
//   - BeansEndpoint DI 容器内省 [v1.9.4]
//   - CachesEndpoint 缓存内省 [v1.9.4]
//   - ShutdownEndpoint 优雅停机 [v1.9.4]
//   - CircuitBreaker 熔断器
//   - Validator 输入校验
//   - ORM 数据库 + MQ 消息队列
//
// 启动后访问:
//   GET http://localhost:8080/api/health        → 健康检查
//   GET http://localhost:8080/api/health/liveness → 存活探针
//   GET http://localhost:8080/metrics            → Prometheus 指标
//   GET http://localhost:8080/actuator/info      → 应用信息
//   GET http://localhost:8080/actuator/loggers   → 日志级别
//   GET http://localhost:8080/actuator/env       → 环境配置 [v1.9.4]
//   GET http://localhost:8080/actuator/mappings  → 路由映射 [v1.9.4]
//   GET http://localhost:8080/actuator/metrics   → 指标列表 [v1.9.4]
//   GET http://localhost:8080/actuator/threaddump → 线程转储 [v1.9.4]
//   GET http://localhost:8080/actuator/beans     → DI 容器 Bean 列表 [v1.9.4]
//   GET http://localhost:8080/actuator/caches    → 缓存列表 [v1.9.4]
//   POST http://localhost:8080/actuator/shutdown → 优雅停机 [v1.9.4]
//   POST http://localhost:8080/api/users         → 创建用户(JSON body)
//   GET http://localhost:8080/api/users          → 用户列表

#include <QCoreApplication>
#include <QPointer>
#include <QTimer>
#include <iostream>

// SoulCoreKit 聚合头文件
#include "soul/core/scaffold.h"
#include "soul/core/module.h"
#include "soul/server/http_server.h"
#include "soul/server/middleware.h"
#include "soul/server/health.h"
#include "soul/server/info_endpoint.h"
#include "soul/server/loggers_endpoint.h"
#include "soul/server/env_endpoint.h"
#include "soul/server/mappings_endpoint.h"
#include "soul/server/metrics_endpoint.h"
#include "soul/server/threaddump_endpoint.h"
#include "soul/server/beans_endpoint.h"
#include "soul/server/caches_endpoint.h"
#include "soul/server/shutdown_endpoint.h"
#include "soul/observability/prometheus_exporter.h"
#include "soul/observability/tracing.h"
#include "soul/network/policy/circuit_breaker.h"
#include "soul/network/policy/rate_limiter.h"
#include "soul/validation/validator.h"
#include "soul/logging/log_macros.h"
#include "soul/orm/entity.h"
#include "soul/orm/query_wrapper.h"
#include "soul/orm/sqlite_repository.h"
#include "soul/orm/entities.h"
#include "soul/data/connection_pool.h"
#include "soul/mq/mq_factory.h"
#include "soul/mq/imq_connection.h"
#include "soul/mq/imq_producer.h"
#include "soul/utils/json/json_helper.h"

using namespace sc;
using namespace sc::server;
using namespace sc::network;
using namespace sc::observability;
using namespace sc::validation;
using namespace sc::orm;
using namespace sc::data;
using namespace sc::mq;

// ============================================================================
// ServerModule — 将 HttpServer 包装为 Scaffold 模块
// ============================================================================
class ServerModule : public Module {
public:
    ServerModule() : Module("ServerModule") {}

    std::vector<std::string> dependsOn() const override { return {}; }
    int priority() const override { return 10; }

    Result<void> init() override {
        m_server = std::make_shared<HttpServer>();
        m_health = std::make_shared<HealthEndpoint>();

        // 初始化数据库连接池(示例使用 SQLite 内存数据库)
        sc::data::ConnectionConfig dbConfig;
        dbConfig.type = DatabaseType::SQLite;
        dbConfig.database = ":memory:";
        m_dbPool = std::make_shared<DefaultDbConnectionPool>(dbConfig, 2, 5);
        return {};
    }

    Result<void> onStart() override {
        // ============================================================
        // 1. 中间件链(按顺序注册)
        // ============================================================

        // 1.1 请求日志
        m_server->use(std::make_shared<LoggingMiddleware>());

        // 1.2 限流(100 QPS, 令牌桶, 排除健康检查)
        auto limiter = std::make_shared<RateLimiter>(
            RateLimiter::Algorithm::TokenBucket, 100.0);
        auto rl = std::make_shared<RateLimitMiddleware>(limiter);
        rl->addExcludePath("/api/health");
        rl->addExcludePath("/api/health/liveness");
        rl->addExcludePath("/metrics");
        rl->addExcludePath("/actuator/info");
        // [v1.9.4] 诊断端点免限流,避免运维查询被误判为攻击
        rl->addExcludePath("/actuator/metrics");
        rl->addExcludePath("/actuator/threaddump");
        rl->addExcludePath("/actuator/beans");
        rl->addExcludePath("/actuator/caches");
        rl->addExcludePath("/actuator/shutdown");
        m_server->use(rl);

        // 1.3 CORS(开发环境允许所有来源)
        m_server->use(std::make_shared<CorsMiddleware>());

        // 1.4 分布式追踪(V1.9.3 新增: W3C Trace Context)
        // 在生产环境中可通过 setEnabled(false) 关闭追踪以降低开销
        Tracer::instance().setEnabled(true);

        // ============================================================
        // 2. Actuator 端点
        // ============================================================

        // /actuator/info — 应用信息
        InfoEndpoint::setAppName("SoulCoreKit Demo");
        InfoEndpoint::setAppVersion("1.9.4");
        m_server->get("/actuator/info", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(InfoEndpoint::toJson());
        });

        // /actuator/loggers — 日志级别
        m_server->get("/actuator/loggers", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(LoggersEndpoint::getAllLevels());
        });

        // /actuator/env — 环境配置 [v1.9.4]
        m_server->get("/actuator/env", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(EnvironmentEndpoint::toJson());
        });

        // /actuator/mappings — 路由映射 [v1.9.4]
        // QPointer 生命周期安全: server 销毁时自动置 null,防止 UAF [v1.9.4]
        QPointer<HttpServer> serverPtr(m_server.get());
        m_server->get("/actuator/mappings", [serverPtr](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(MappingsEndpoint::toJson(*serverPtr));
        });

        // /actuator/metrics — 指标列表 + 单指标查询 [v1.9.4]
        m_server->get("/actuator/metrics", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(MetricsEndpoint::listMetricNames());
        });

        // /actuator/threaddump — 线程转储 [v1.9.4]
        m_server->get("/actuator/threaddump", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(ThreadDumpEndpoint::toJson());
        });

        // /actuator/beans — DI 容器 Bean 内省 [v1.9.4]
        m_server->get("/actuator/beans", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(BeansEndpoint::toJson());
        });

        // /actuator/caches — 缓存内省 [v1.9.4]
        m_server->get("/actuator/caches", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(CachesEndpoint::toJson());
        });

        // /actuator/shutdown — 优雅停机 (POST) [v1.9.4]
        // 先返回响应,再异步触发 shutdown,避免响应被连接关闭截断
        m_server->post("/actuator/shutdown", [serverPtr](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(ShutdownEndpoint::toJson());
            // 延迟 100ms 触发 shutdown,确保响应已发回客户端
            // QPointer 自动检测 server 是否存活,防止 UAF [v1.9.4]
            QTimer::singleShot(100, [serverPtr]() {
                if (serverPtr) {
                    serverPtr->shutdown(5000);
                }
            });
        });

        // 注: /actuator/scheduledtasks 端点 [v1.9.4] 需传入 Scheduler 实例,
        //     本示例未启用 Scheduler 模块。启用方式:
        //     m_server->get("/actuator/scheduledtasks",
        //         [&scheduler](const HttpRequest&, HttpResponse& resp) {
        //             resp.setHeader("Content-Type", "application/json");
        //             resp.setBody(ScheduledTasksEndpoint::toJson(scheduler));
        //         });

        // /metrics — Prometheus 指标
        m_server->get("/metrics", [](const HttpRequest&, HttpResponse& resp) {
            resp.setHeader("Content-Type", "text/plain; version=0.0.4");
            resp.setBody(
                sc::observability::PrometheusExporter::exportMetrics());
        });

        // ============================================================
        // 3. 健康检查端点
        // ============================================================

        // 数据库健康检查
        m_health->addIndicator(std::make_shared<DatabaseHealthIndicator>(
            [this]() -> bool {
                if (!m_dbPool) return false;
                auto conn = m_dbPool->acquire(100);
                return conn.isOk();
            }, "database"));

        // /api/health — readiness(所有依赖)
        auto health = m_health;  // 捕获 shared_ptr
        m_server->get("/api/health", [health](const HttpRequest&, HttpResponse& resp) {
            auto report = health->readiness();
            resp.setStatus(report.overall == HealthStatus::UP ? 200 : 503);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(report.toJson());
        });

        // /api/health/liveness — 仅关键依赖
        m_server->get("/api/health/liveness", [health](const HttpRequest&, HttpResponse& resp) {
            auto report = health->liveness();
            resp.setStatus(report.overall == HealthStatus::UP ? 200 : 503);
            resp.setHeader("Content-Type", "application/json");
            resp.setBody(report.toJson());
        });

        // ============================================================
        // 4. 业务路由(带熔断 + 校验)
        // ============================================================

        // 熔断器: 数据库服务, 5 次失败 → 熔断 30 秒
        auto dbBreaker = std::make_shared<CircuitBreaker>("db-service");
        dbBreaker->setFailureThreshold(5).setResetTimeout(30000);

        // POST /api/users — 创建用户
        m_server->post("/api/users", [this, dbBreaker](const HttpRequest& req, HttpResponse& resp) {
            // 分布式追踪: SpanGuard RAII 自动 end(),异常安全
            SpanGuard spanGuard(Tracer::instance().startSpan("createUser"));
            spanGuard->setTag("endpoint", "/api/users");

            // 输入校验
            Validator validator;
            SafeStringOptions opts;
            opts.allowQuotes = false;
            opts.allowSemicolon = false;
            validator.required("username", req.header("X-Username").toStdString(), "用户名不能为空")
                     .safeString("username", req.header("X-Username"),
                                 "用户名包含不安全字符", opts);

            auto result = validator.validate();
            if (!result.isValid()) {
                resp.setStatus(400);
                resp.setHeader("Content-Type", "application/json");
                sc::json::Json err = {{"error", result.firstError()}};
                resp.setBody(sc::json::serialize(err));
                return;
            }

            // 熔断保护
            auto saveResult = dbBreaker->call([&]() -> Result<void> {
                if (!m_dbPool) {
                    return Result<void>::err(Error(ErrorCode::InternalError, "DB not initialized"));
                }
                // 实际业务逻辑: 保存用户到数据库
                return {};
            });

            if (!saveResult.isOk()) {
                resp.setStatus(503);
                resp.setHeader("Content-Type", "application/json");
                sc::json::Json err = {{"error", "Service temporarily unavailable"}};
                resp.setBody(sc::json::serialize(err));
                return;
            }

            resp.setStatus(201);
            resp.setHeader("Content-Type", "application/json");
            sc::json::Json created = {{"status", "created"}};
            resp.setBody(sc::json::serialize(created));
        });

        // GET /api/users — 用户列表(带超时保护)
        m_server->get("/api/users", [dbBreaker](const HttpRequest&, HttpResponse& resp) {
            auto result = dbBreaker->call([&]() -> Result<std::string> {
                return Result<std::string>::ok("[]");
            });

            resp.setHeader("Content-Type", "application/json");
            if (result.isOk()) {
                resp.setBody(QByteArray(result.unwrap().c_str()));
            } else {
                resp.setStatus(503);
                sc::json::Json err = {{"error", "Service unavailable"}};
                resp.setBody(sc::json::serialize(err));
            }
        });

        // ============================================================
        // 5. 启动监听
        // ============================================================
        if (!m_server->listen(QHostAddress::Any, 8080)) {
            return Result<void>::err(Error(ErrorCode::InternalError, "Failed to start HTTP server"));
        }

        SC_INFO("Server started on http://localhost:8080");
        return {};
    }

    void onStop() override {
        if (m_server) {
            m_server->shutdown(5000);  // 优雅关闭, 等待 5 秒
        }
        Logger::instance().flush();
    }

    void cleanup() override {
        m_server.reset();
        m_health.reset();
        m_dbPool.reset();
    }

    // 公开访问器(供外部使用)
    std::shared_ptr<HttpServer> server() { return m_server; }
    std::shared_ptr<HealthEndpoint> health() { return m_health; }

private:
    std::shared_ptr<HttpServer> m_server;
    std::shared_ptr<HealthEndpoint> m_health;
    std::shared_ptr<DefaultDbConnectionPool> m_dbPool;
};

// ============================================================================
// main — 脚手架入口
// ============================================================================
int main(int argc, char* argv[]) {
    // 1. 创建 Scaffold(自动管理 QCoreApplication)
    Scaffold scaffold(argc, argv);

    // 2. 注册 Server 模块
    ServerModule serverModule;
    scaffold.use(serverModule);

    // 3. 设置应用信息
    InfoEndpoint::setStartupTime(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());

    SC_INFO("SoulCoreKit v1.9.4 Full Stack Demo starting...");

    // 4. 启动(自动拓扑排序 → init → onStart → 事件循环)
    return scaffold.run();
    // 5. 退出时 Scaffold::~Scaffold() 自动调用 shutdown() → onStop → cleanup
}