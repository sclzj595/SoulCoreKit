# 04 — 核心功能体系

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 4.1 启动流程 (对标 SpringBoot)

```
Application
    │
    ▼ configure()                        ← 虚拟钩子，用户自定义配置
    │
    ▼ printBanner()                      ← ASCII Art 横幅
    │
    ▼ loadConfiguration()                ← YAML 加载 + Profile 分层
    │
    ▼ registerModules()                  ← 用户注册模块
    │
    ▼ scanAndRegisterModules()           ← 自动扫描 ModuleRegistry
    │
    ▼ topoSortWithPriority()             ← 拓扑排序 + 优先级排序
    │
    ▼ initializeModules()                ← 顺序 init，失败自动回滚
    │
    ▼ startModules()                     ← 顺序 start，失败自动回滚
    │
    ▼ startServices()                    ← 启动 HttpServer 等
    │
    ▼ onStarted()                        ← 启动完成回调
    │
    ▼ event loop                         ← QCoreApplication::exec()
    │
    ▼ onStopping()                       ← 停止前回调
    │
    ▼ stopModules()                      ← 逆序停止
    │
    ▼ cleanupModules()                   ← 逆序清理
```

### 生命周期对照表

| 阶段 | 方法 | 对标 SpringBoot |
|------|------|----------------|
| 初始化 | `Module::init()` | `@PostConstruct` |
| 启动 | `Module::onStart()` | `ContextRefreshedEvent` |
| 停止 | `Module::onStop()` | `ContextClosedEvent` |
| 清理 | `Module::cleanup()` | `@PreDestroy` |

### 状态机

```
Created → Starting → Running → Stopping → Stopped
```

---

## 4.2 CS 请求链路

```
User Action → CsRouter::navigate() → RouteMatcher::match()
    → CsController::dispatch() (强类型优先)
    → CsService::execute() → Repository
    → Result<T> → CsViewModel → Qt Widgets
```

### 分层职责

```
┌──────────────┐
│   View       │  QWidget — 纯 UI 渲染
├──────────────┤
│   ViewModel  │  UI State 管理 (Q_PROPERTY)
├──────────────┤
│  Controller  │  Command / Request 分发
├──────────────┤
│   Service    │  业务逻辑
├──────────────┤
│  Repository  │  数据访问 (CRUD)
└──────────────┘
```

---

## 4.3 DI 容器

- **三种生命周期**: `Singleton` / `Scoped` / `Transient`
- **命名绑定**: `bindNamed<T>()` 支持 `@Qualifier`
- **作用域管理**: `createScope()` / `disposeScope()`
- **线程安全**: DCLP (Double-Checked Locking Pattern)
- **v2.5.1 修复**: `shared_ptr<void>` 存储单例，消除 use-after-free

---

## 4.4 数据访问层

```
IRepository<T,Id>           ← 唯一 Repository 抽象 (data 模块)
    ├── SQLiteRepository    ← SQLite 实现
    ├── MySQLRepository     ← MySQL 实现
    ├── PostgreSQLRepository← PostgreSQL 实现
    ├── MemoryRepository    ← 内存实现
    └── JsonRepository      ← JSON 文件实现

ORM 层 (MyBatis-Plus 风格):
    Entity<T>               ← CRTP 实体基类
    BaseRepository<T>       ← 模板方法模式
    QueryWrapper<T>         ← 类型安全查询构建
    ISqlDialect             ← 多数据库方言
    CachedRepository<T>     ← 装饰器模式
    MigrationManager         ← Schema 迁移
```

---

## 4.5 网络层 (4 层架构)

```
soul_network_core     → INetwork / IInterceptor / INetworkPolicy / ICodec
soul_network_policy   → Retry / Timeout / CircuitBreaker / RateLimiter / Heartbeat
soul_network_http     → HttpClient / ConnectionPool / Monitor / Interceptor
soul_network_protocol → WebSocket / TCP / MQTT / Bluetooth / Serial / NamedPipe
```

---

## 4.6 可观测性 (Actuator 端点)

| 端点 | 路径 | 对标 SpringBoot |
|------|------|----------------|
| Health | `/actuator/health` | `HealthEndpoint` |
| Info | `/actuator/info` | `InfoEndpoint` |
| Metrics | `/actuator/metrics` | `MetricsEndpoint` + Prometheus |
| Environment | `/actuator/env` | `EnvEndpoint` |
| Beans | `/actuator/beans` | `BeansEndpoint` |
| Caches | `/actuator/caches` | `CachesEndpoint` |
| Loggers | `/actuator/loggers` | `LoggersEndpoint` |
| Mappings | `/actuator/mappings` | `MappingsEndpoint` |
| Thread Dump | `/actuator/threaddump` | `ThreadDumpEndpoint` |
| Scheduled Tasks | `/actuator/scheduledtasks` | `ScheduledTasksEndpoint` |
| Shutdown | `/actuator/shutdown` | `ShutdownEndpoint` |

---

## 4.7 配置管理

- **格式**: YAML (`application.yml`)
- **Profile**: `application-{dev|test|prod}.yml` 分层覆盖
- **优先级**: 命令行 > Profile YAML > 基础 YAML > 默认值
- **自动装配**: `conditionalOnProperty`, `conditionalOnProfile`, `conditionalOnDatabase` 等
- **远程配置**: Etcd / Nacos 数据源支持

---

## 4.8 认证授权

- **OAuth2 / OIDC**: 完整认证流程
- **Token 管理**: JWT / Bearer Token
- **权限系统**: RBAC 权限模型
- **安全存储**: 加密存储敏感数据

---

## 4.9 消息队列

- **抽象层**: `IAmqpBackend` 统一接口
- **内存模拟**: `InMemoryAmqpBackend` (测试用)
- **真实集成**: `AmqpCppBackend` (RabbitMQ via amqpcpp)
- **Exchange 类型**: Direct / Fanout / Topic
- **QoS**: prefetchCount 控制
- **消息确认**: ack / nack / reject