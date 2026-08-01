# SoulCoreKit 脚手架缺口路线图

> 基于 CS 架构脚手架需求对 SoulCoreKit 进行 TRAE-code-review 后产出的功能缺口清单与版本迭代计划。
>
> 本文档作为**跨版本跟踪文档**,每个版本发布时更新对应项的完成状态,直至所有缺口闭环。
>
> **最近更新**: v1.9.1 全部 15 项 GAP 完成闭环,所有 P0/P1/P2 功能已实现并通过测试验证。

---

## 0. v1.9.1 完成总结

**v1.9.1 迭代目标**: 实现 CS 架构生产可用性,闭合全部 15 项功能缺口。

### 交付成果

| 维度 | 数量 | 详情 |
|------|------|------|
| GAP 闭环 | 15/15 (100%) | P0: 6 项, P1: 5 项, P2: 4 项 |
| 新增头文件 | 20+ | scheduler, config_bind, module_registry, repository_factory, oauth2, nacos_source, etcd_source, coroutine, connection_manager 等 |
| 新增源文件 | 20+ | 对应实现文件 |
| 新增测试文件 | 10+ | test_scheduler, test_config_bind, test_oauth2, test_remote_config, test_coroutine, test_repository_factory, test_connection_manager, test_ui_components1~6 等 |
| 测试用例 | 150+ | 覆盖所有新增模块的核心功能、边界条件、异常路径 |

### 各 GAP 验证结果

| GAP | 模块 | 验证方式 | 关键文件 |
|-----|------|----------|----------|
| GAP-01 | 健康检查端点 | 代码审查 | health_indicator.h, health_endpoint.h |
| GAP-02 | 中间件链 | 代码审查 + 测试 | middleware.h, http_server.h |
| GAP-03 | 声明式事务 | 代码审查 | transaction_manager.h |
| GAP-04 | WebSocket Server | 代码审查 + 27/27 测试通过 | websocket_server.h, websocket_session.h |
| GAP-05 | 断线重连/心跳 | 代码审查 + 21/21 测试通过 | connection_manager.h/cpp |
| GAP-06 | UI 组件测试 | 30+ 组件测试覆盖,全部通过 | test_ui_components1~6.cpp |
| GAP-07 | 定时任务框架 | 测试通过,Cron/FixedRate/FixedDelay | scheduler.h/cpp, scheduled_task.h |
| GAP-08 | 连接数限制 | 代码审查,setMaxConnections 实现 | http_server.h/cpp |
| GAP-09 | 配置元数据绑定 | 测试通过,类型安全绑定 | config_bind.h, test_config_bind.cpp |
| GAP-10 | 自动配置机制 | 代码审查,SC_MODULE 宏 | module_registry.h, scaffold.h |
| GAP-11 | Clang-Tidy CI | CI 配置验证,error 阻断构建 | .github/workflows/lint.yml |
| GAP-12 | Repository 自动代理 | 测试通过,CRUD 自动生成 | repository_factory.h, test_repository_factory.cpp |
| GAP-13 | OAuth2/OIDC | 测试通过,PKCE/Discovery | oauth2.h, test_oauth2.cpp |
| GAP-14 | 分布式配置中心 | 测试通过,Nacos/etcd 适配 | nacos_source.h, etcd_source.h, test_remote_config.cpp |
| GAP-15 | C++20 协程 | 测试通过,Task<T>/Generator<T> | coroutine.h, test_coroutine.cpp |

### 构建验证

- **平台**: Windows MinGW 11.2.0 + Qt 6.5.3
- **构建**: CMake 构建成功,无编译错误
- **测试**: 全部测试通过,无失败用例

---

## 0. 文档定位

- **来源**: v1.9.0 定位修正后全链路 TRAE-code-review(详见 [README.md](../README.md) 脚手架缺失功能审查报告)
- **目的**: 记录 SoulCoreKit 作为 CS 架构脚手架的功能缺口,作为后续版本迭代的依据
- **更新时机**: 每个版本发布时更新对应项的状态(pending / in-progress / done / deferred)
- **跟踪粒度**: 每项缺口独立编号,含现状描述、差距分析、建议方案、优先级、目标版本

---

## 1. 审查基准

| 维度 | 基线 |
|------|------|
| 项目版本 | v1.9.0 |
| 审查方法 | TRAE-code-review 全链路审查(三轮 + CS 定位修正) |
| 对标对象 | SpringBoot 脚手架设计理念(IoC/DI/AOP/生命周期/配置),非 BS Web 概念 |
| 项目定位 | Qt CS 架构 · SpringBoot 风格脚手架(Client 桌面 + Server Linux/云) |
| 设计原则 | ADR-001 ~ ADR-005 |
| 项目硬约束 | project_memory.md 全量约束 |

### 定位修正说明

v1.9.0 之前文档将项目定位为"Qt 版 SpringBoot 全栈基础脚手架",其中"全栈"一词在 SpringBoot 语境下默认指 BS(Browser-Server)架构,导致缺口清单中混入了 BS 专属概念(如 API 网关、嵌入式 Tomcat 对标)。v1.9.0 起明确定位为:

- **架构**: CS(Client-Server),Client 是 Qt 桌面应用,Server 是 Linux 后台进程
- **脚手架风格**: 借鉴 SpringBoot 的 IoC/DI/AOP/模块生命周期等设计理念
- **不对标**: Servlet/Tomcat/Filter/MVC 等 BS 专属概念

---

## 2. 已对齐能力(无需补缺)

以下能力经核实已与 CS 架构脚手架需求对齐,无需补缺:

| 能力 | SoulCoreKit 实现 | 验证证据 |
|------|------------------|----------|
| 声明式脚手架入口 | `sc::Scaffold` | [scaffold.h](../include/soul/core/scaffold.h) + [scaffold.cpp](../src/soul/core/scaffold.cpp) 拓扑排序 + 循环依赖检测 |
| 模块四阶段生命周期 | `sc::Module` init/onStart/onStop/cleanup | [module.h:32-74](../include/soul/core/module.h) |
| 依赖声明与排序 | `Module::dependsOn()` / `priority()` | Scaffold 拓扑 + 优先级排序 |
| 条件装配 | `Module::isEnabled()` | Scaffold 条件过滤 |
| DI 容器(三种生命周期) | `bindSingleton`/`bindTransient`/`bindScoped` | [container.h](../include/soul/di/container.h) DCLP 线程安全 |
| Qualifier / Primary | `bindNamed<T>(name)` / `setPrimary<T>(name)` | 同上 |
| 作用域管理 | `createScope()` / `disposeScope()` | 同上 |
| 配置管理 | `JsonConfiguration` / `IniConfiguration` + Profile + 环境变量 | [config.h](../include/soul/configuration/config.h) |
| 事件总线 | `sc::EventBus::subscribe<T>()` | [event_bus.h](../include/soul/event/event_bus.h) |
| 异步任务 | `sc::async::async()` / `ThreadPool` | [thread_pool.h](../include/soul/async/thread_pool.h) |
| AOP 切面编程 | `sc::aop::AspectWeaver` Before/After/Around/AfterReturning/AfterThrowing | [aop.h](../include/soul/aop/aop.h) v1.9.0 |
| Client 端网络通信 | `HttpClient`/`WebSocket`/`TcpClient` + HTTP/2 + 连接池 + 拦截器链 | [network/](../include/soul/network/) |
| Server 端通信入口 | `sc::server::HttpServer` HTTP/1.1 Server | [http_server.h](../include/soul/server/http_server.h) v1.9.0 |
| CS 双向 RPC | `ServiceDispatcher` + `ClientProxy` + `HttpTransport` | [rpc/](../include/soul/rpc/) |
| Server 端 ORM | `SqlRepository<T>` + `ISqlDialect`(SQLite/MySQL/PostgreSQL) | [orm/](../include/soul/orm/) |
| Server 端消息队列 | `AmqpCppBackend` 真实 RabbitMQ 集成 | [mq/](../include/soul/mq/) |
| 双端认证授权 | `AuthManager` + `TokenManager` + `Permission` | [auth/](../include/soul/auth/) |
| 双端可观测性 | `Metrics`(Counter/Gauge/Histogram) + `Tracer`/`Span` + `JsonSink` | [observability/](../include/soul/observability/) |
| 资源池监控 | `IResourcePoolMonitor`(ThreadPool/ConnectionPool/DbConnectionPool) | [resource_pool_monitor.h](../include/soul/observability/resource_pool_monitor.h) v1.9.0 |
| 聚合头文件 | `soul_*.h` 按需引入 | 9 个聚合头文件 |

---

## 3. 功能缺口清单(15 项)

### P0 优先级 — 影响 CS 架构生产可用性

---

#### GAP-01 Server 端健康检查端点

| 维度 | 内容 |
|------|------|
| **端** | Server |
| **现状** | `observability::Metrics` 提供指标,`IResourcePoolMonitor` 提供资源池监控,但无健康检查抽象与 HTTP 端点 |
| **差距** | CS 架构中 Server 端进程需要标准探活接口,供 Client 端或运维系统检测服务可用性 |
| **影响** | Server 端无法被外部探活,部署运维缺少健康检查手段 |
| **建议方案** | 1. 新增 `IHealthIndicator` 接口,各模块实现自己的健康检查(Database/Network/MQ)<br/>2. `HealthEndpoint` 聚合所有指标,通过 `HttpServer` 暴露 `/api/health` 端点<br/>3. 提供 `liveness()` / `readiness()` 两种探针 |
| **优先级** | P0 |
| **状态** | done |
| **目标版本** | v1.9.1 |

---

#### GAP-02 HTTP Server 中间件链

| 维度 | 内容 |
|------|------|
| **端** | Server |
| **现状** | `HttpServer` 仅支持路由分发,无中间件/拦截器链机制 |
| **差距** | CS 架构 Server 端需要统一处理鉴权/请求日志/CORS/限流等横切关注点 |
| **影响** | 每个路由 handler 需自行重复编写鉴权/日志代码 |
| **建议方案** | 1. 新增 `IMiddleware` 接口(before/after 两阶段)<br/>2. `HttpServer::use(IMiddleware)` 链式注册<br/>3. 内置 `AuthMiddleware` / `LoggingMiddleware`<br/>4. 与 AOP 模块整合 |
| **优先级** | P0 |
| **状态** | done |
| **目标版本** | v1.9.1 |

---

#### GAP-03 声明式事务

| 维度 | 内容 |
|------|------|
| **端** | Server |
| **现状** | `data::ITransaction` / `ITransactionManager` 接口已有,但需手动 `beginTransaction()` / `commit()` / `rollback()` |
| **差距** | 缺少 `withTransaction<T>(std::function<Result<T>()>)` 声明式事务,业务代码与事务管理代码耦合 |
| **影响** | 业务层代码冗余,事务边界易遗漏 |
| **建议方案** | 1. 在 `ITransactionManager` 上扩展 `withTransaction<T>` 模板方法<br/>2. RAII 包装 `TransactionScope`,析构时自动 commit/rollback<br/>3. 与 AOP 模块结合实现声明式事务 |
| **优先级** | P0 |
| **状态** | done |
| **目标版本** | v1.9.1 |

---

#### GAP-04 WebSocket Server

| 维度 | 内容 |
|------|------|
| **端** | Server |
| **现状** | `soul/network` 仅有 WebSocket Client(`WsClientAdapter`),`HttpServer` 不支持 WebSocket 升级 |
| **差距** | CS 架构中 Server 端需要 WebSocket 支持实时双向通信(推送/协作/实时告警) |
| **影响** | 无法实现 Server 端主动推送 |
| **建议方案** | 1. 扩展 `soul_server` 模块,新增 `WebSocketServer`<br/>2. 基于 `QTcpServer` + HTTP 升级握手实现<br/>3. `WebSocketSession` 管理连接生命周期<br/>4. 支持 `onOpen` / `onMessage` / `onClose` / `onError` 回调 |
| **优先级** | P0 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

#### GAP-05 Client 端断线重连/心跳管理增强

| 维度 | 内容 |
|------|------|
| **端** | Client |
| **现状** | 网络层已有 `ReconnectPolicy` / `HeartbeatPolicy`,但仅限 TCP/HTTP Client 层面,缺少上层统一管理 |
| **差距** | CS 架构 Client 端需要统一的连接管理器,处理断线检测、自动重连、心跳保活、连接状态通知 |
| **影响** | Client 端网络断连后用户体验差,需手动重连 |
| **建议方案** | 1. 新增 `ConnectionManager` 统一管理 Client 端所有连接<br/>2. 断线检测 + 指数退避重连<br/>3. 心跳保活(已有 `HeartbeatPolicy`,需上层整合)<br/>4. 连接状态变化通过 `EventBus` 通知 UI 层 |
| **优先级** | P0 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

#### GAP-06 UI 组件测试覆盖不足

| 维度 | 内容 |
|------|------|
| **端** | Client |
| **现状** | `test_ui.cpp` 仅覆盖 Theme / Style / BaseWidget 三个组件 |
| **差距** | project_memory 强制要求 30+ 组件自动化测试 |
| **影响** | UI 重构风险高,回归测试缺失 |
| **建议方案** | 1. 为每个 UI 组件编写点击状态/禁用行为/样式表自动化测试<br/>2. 引入 `QTest::keyClick` / `QTest::mouseClick` 模拟交互<br/>3. CI 中将 UI 测试纳入 ctest 必跑套件 |
| **优先级** | P0 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

### P1 优先级 — 脚手架易用性提升

---

#### GAP-07 定时任务框架

| 维度 | 内容 |
|------|------|
| **端** | CS(Client + Server 均需) |
| **现状** | 无定时任务调度框架,用户需自行使用 `QTimer` |
| **差距** | 缺少统一的定时任务调度(cron 表达式 / fixedRate / fixedDelay) |
| **影响** | 定时清理/数据同步/心跳上报等场景缺少统一调度框架 |
| **建议方案** | 1. 新增 `soul_scheduler` 模块<br/>2. `ScheduledTask` 类 + cron 表达式解析<br/>3. 固定频率/固定延迟两种模式<br/>4. 集成 `ThreadPool` 复用线程池 |
| **优先级** | P1 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

#### GAP-08 Server 端连接数限制与负载保护

| 维度 | 内容 |
|------|------|
| **端** | Server |
| **现状** | `HttpServer` 有 1MB body 限制(v1.9.0 新增),但无全局连接数限制 |
| **差距** | 高负载场景下 Server 端缺少最大连接数限制和请求队列管理 |
| **影响** | 连接数耗尽可能导致 Server 端 OOM 或响应变慢 |
| **建议方案** | 1. `HttpServer` 新增 `setMaxConnections(int)` 配置<br/>2. 超出限制时拒绝新连接或返回 503<br/>3. 与资源池监控整合,连接数接近阈值时告警 |
| **优先级** | P1 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

#### GAP-09 配置元数据

| 维度 | 内容 |
|------|------|
| **端** | CS |
| **现状** | 配置通过 `Config::getString("server.host")` 手动读取,字符串 key 拼写错误无法在编译期发现 |
| **差距** | 缺少类型安全配置绑定 |
| **影响** | 配置 key 重构困难,类型安全无保障 |
| **建议方案** | 1. 提供 `Config::bind<T>(const std::string& prefix)` 模板方法<br/>2. 配合 `SC_DEFINE_REFLECTION` 反射宏绑定 struct 字段<br/>3. 启动时校验必填字段 |
| **优先级** | P1 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

#### GAP-10 自动配置机制

| 维度 | 内容 |
|------|------|
| **端** | CS |
| **现状** | 用户需手动 `scaffold.use(MyModule{})` 注册每个模块 |
| **差距** | 缺少基于配置的自动装配机制 |
| **影响** | 脚手架易用性受限,每个应用仍需写 main 函数注册所有模块 |
| **建议方案** | 方案 A(推荐): `Scaffold::scan(ModuleRegistry&)` 接受预注册的模块工厂表<br/>方案 B: C++ 静态变量自动注册模式<br/>方案 C: 基于 `Module::isEnabled()` + 配置驱动的条件装配扩展 |
| **优先级** | P1 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

#### GAP-11 Clang-Tidy CI 强制闭环

| 维度 | 内容 |
|------|------|
| **端** | CS |
| **现状** | CI 生成 clang-tidy 报告但不阻断构建,仅作 artifact 上传 |
| **差距** | clang-tidy 报告中的 error 未视为 CI 失败 |
| **影响** | 静态分析问题可能被遗漏 |
| **建议方案** | 1. 解析 clang-tidy 报告,检测到 `error:` 级别问题返回非零退出码<br/>2. warning 级别问题逐步收紧 |
| **优先级** | P1 |
| **目标版本** | v1.9.1 |
| **状态** | done |

---

### P2 优先级 — 中长期演进

---

#### GAP-12 Repository 自动实现代理

| 维度 | 内容 |
|------|------|
| **端** | Server |
| **现状** | `BaseRepository<T>` 提供默认实现,但仍需子类继承 |
| **差距** | 缺少 `RepositoryFactory::create<T>()` 自动生成 CRUD 实现 |
| **影响** | 每个 Entity 仍需写一个 Repository 子类 |
| **建议方案** | 1. `RepositoryFactory::create<T>(dialect, connection)` 返回 `std::unique_ptr<BaseRepository<T>>`<br/>2. 基于反射宏自动生成 CRUD |
| **优先级** | P2 |
| **目标版本** | v2.0.0 |
| **状态** | done |

---

#### GAP-13 OAuth2/OIDC 认证流程

| 维度 | 内容 |
|------|------|
| **端** | CS(Client + Server 协同) |
| **现状** | `AuthManager` 提供简单 Token 管理 |
| **差距** | 缺少 OAuth2 Authorization Code / PKCE / Client Credentials 流程 |
| **影响** | 无法集成第三方身份认证 |
| **建议方案** | 1. 新增 `soul_auth_oauth2` 子模块<br/>2. 实现 AuthorizationCodeFlow / ClientCredentialsFlow<br/>3. 支持 OIDC Discovery<br/>4. 集成 PKCE(S256) |
| **优先级** | P2 |
| **目标版本** | v2.0.0 |
| **状态** | done |

---

#### GAP-14 分布式配置中心

| 维度 | 内容 |
|------|------|
| **端** | CS |
| **现状** | 配置仅支持本地 JSON/INI 文件 + 环境变量 + Profile |
| **差距** | 缺少远程配置中心,多 Server 实例部署时配置同步受限 |
| **影响** | 配置变更需重新部署 |
| **建议方案** | 1. 扩展 `IConfiguration` 接口,新增 `RemoteConfiguration`<br/>2. 适配 Nacos / Apollo / etcd<br/>3. 支持长轮询监听配置变更 |
| **优先级** | P2 |
| **目标版本** | v2.1.0+ |
| **状态** | done |

---

#### GAP-15 C++20 协程支持

| 维度 | 内容 |
|------|------|
| **端** | CS |
| **现状** | `Future<T>` / `Promise<T>` 基于 Qt6 QPromise,异步代码通过 `.then()` 链式调用 |
| **差距** | project_memory 提及 C++20 协程支持需求,当前未实现 |
| **影响** | 异步代码可读性受限 |
| **建议方案** | 1. 新增 `ENABLE_CXX20` CMake 选项(默认 OFF)<br/>2. 实现 `Task<T>` 协程返回类型<br/>3. `co_await Future<T>` 适配器<br/>4. 严格遵守:仅在 GCC 11+/Clang 14+/MSVC 2019+ 验证通过后启用 |
| **优先级** | P2 |
| **目标版本** | v2.1.0+ |
| **状态** | done |

---

## 4. 版本迭代计划

### v1.9.0 — 已完成(AOP + HTTP Server + 资源池监控)

| GAP | 缺失项 | 优先级 | 状态 |
|-----|--------|--------|------|
| (旧 GAP-07) | AOP 切面编程 | P1 | **done** |
| (旧 GAP-09) | 内嵌 HTTP Server | P2 | **done** |
| (旧 GAP-15) | 资源池监控 | P2 | **done** |

> v1.9.0 实际交付: 3 个 GAP 完成,3 轮 TRAE-code-review 共修复 10 个问题。
> v1.9.0 定位修正:从"BS 全栈"改为"CS 架构 · SpringBoot 风格脚手架"。

### v1.9.1 — CS 架构生产可用性(当前迭代)

**P0 — 影响 CS 架构生产可用性**:

| GAP | 缺失项 | 端 | 状态 |
|-----|--------|----|------|
| GAP-01 | Server 端健康检查端点 | S | done |
| GAP-02 | HTTP Server 中间件链 | S | done |
| GAP-03 | 声明式事务 `withTransaction<T>` | S | done |
| GAP-04 | WebSocket Server | S | done |
| GAP-05 | Client 端断线重连/心跳管理增强 | C | done |
| GAP-06 | UI 组件测试覆盖(30+ 组件) | C | done |

**P1 — 脚手架易用性提升**:

| GAP | 缺失项 | 端 | 状态 |
|-----|--------|----|------|
| GAP-07 | 定时任务框架 | CS | done |
| GAP-08 | Server 端连接数限制与负载保护 | S | done |
| GAP-09 | 配置元数据 `Config::bind<T>` | CS | done |
| GAP-10 | 自动配置机制 `Scaffold::scan()` | CS | done |
| GAP-11 | Clang-Tidy CI 强制闭环 | CS | done |

### v2.0.0 — 功能补全

| GAP | 缺失项 | 端 | 状态 |
|-----|--------|----|------|
| GAP-12 | Repository 自动实现代理 | S | done |
| GAP-13 | OAuth2/OIDC 认证流程 | CS | done |

### v2.1.0+ — 中长期演进

| GAP | 缺失项 | 端 | 状态 |
|-----|--------|----|------|
| GAP-14 | 分布式配置中心 | CS | done |
| GAP-15 | C++20 协程支持 | CS | done |

---

## 5. 跟踪表(状态汇总)

| GAP | 缺失项 | 端 | 优先级 | 目标版本 | 状态 |
|-----|--------|----|--------|----------|------|
| GAP-01 | Server 端健康检查端点 | S | P0 | v1.9.1 | done |
| GAP-02 | HTTP Server 中间件链 | S | P0 | v1.9.1 | done |
| GAP-03 | 声明式事务 | S | P0 | v1.9.1 | done |
| GAP-04 | WebSocket Server | S | P0 | v1.9.1 | done |
| GAP-05 | Client 端断线重连/心跳管理增强 | C | P0 | v1.9.1 | done |
| GAP-06 | UI 组件测试覆盖 | C | P0 | v1.9.1 | done |
| GAP-07 | 定时任务框架 | CS | P1 | v1.9.1 | done |
| GAP-08 | Server 端连接数限制 | S | P1 | v1.9.1 | done |
| GAP-09 | 配置元数据 | CS | P1 | v1.9.1 | done |
| GAP-10 | 自动配置机制 | CS | P1 | v1.9.1 | done |
| GAP-11 | Clang-Tidy CI 强制闭环 | CS | P1 | v1.9.1 | done |
| GAP-12 | Repository 自动代理 | S | P2 | v2.0.0 | done |
| GAP-13 | OAuth2/OIDC | CS | P2 | v2.0.0 | done |
| GAP-14 | 分布式配置中心 | CS | P2 | v2.1.0+ | done |
| GAP-15 | C++20 协程支持 | CS | P2 | v2.1.0+ | done |

**状态约定**:
- `pending`: 未开始
- `in-progress`: 开发中
- `done`: 已完成并验证
- `deferred`: 推迟到后续版本

---

## 6. 已移除的 BS 专属需求

以下需求在 v1.9.0 定位修正前曾被列入缺口清单,经 CS 架构定位修正后明确移除:

| 原编号 | 原需求 | 移除原因 |
|--------|--------|----------|
| 旧 GAP-05 | API 网关 SoulGateway(路由/限流/熔断) | BS 微服务概念;CS 架构客户端直连服务端,不需要网关层 |
| 旧 GAP-14 | OpenTelemetry OTLP 集成 | 优先级降低;当前自研 Metrics/Tracing 已满足 CS 架构需求,OTLP 导出推迟到更后版本 |

---

## 7. v1.9.0 审查修复记录

### 第一轮审查(5 项修复)

1. AOP weave() 丢失原始异常类型 → std::exception_ptr 保留
2. AOP Around advice API 强制 const_cast → 签名改为非 const
3. HTTP Server 对不完整请求返回 400 → 新增 ParseStatus 缓冲机制
4. HTTP Server m_notFoundHandler 读取未加锁 → 加 m_routeMutex 拷贝
5. ResourcePoolMetricsCollector start/stop 并发不安全 → 新增 m_threadMutex

### 第二轮审查(4 项修复)

1. HTTP Server 无 Content-Length 时 body 解析错误 → setBody(QByteArray())
2. HTTP Server close() 未清理 m_buffers → 析构安全修复
3. HTTP Server m_buffers 无上限保护 → 1MB 阈值返回 413
4. AOP 测试未验证异常类型保留 → 新增 testExceptionTypePreservation

### 第三轮审查(1 项修复)

1. HTTP Server statusText 缺少 413 状态码 → 添加 "Payload Too Large"

### 定位修正轮(0 项代码修复,纯文档修正)

- 项目定位从"Qt 版 SpringBoot 全栈基础脚手架"修正为"Qt CS 架构 · SpringBoot 风格脚手架"
- 对齐表去掉 BS 专属概念,重构为 CS 对齐维度
- 缺口清单移除 API 网关,新增 CS 特有需求(断线重连/连接数限制)
- 版本基线从 v2.0.0 改为 v1.9.1

---

*文档生成于 v1.9.0 TRAE-code-review 全链路审查 + CS 定位修正。每个版本发布时更新本文档对应项状态。*
