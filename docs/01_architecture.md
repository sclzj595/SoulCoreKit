# SoulCoreKit Architecture Specification v2.6.0

> **状态**: Frozen  
> **定位**: 面向 CS + BS 场景的 C++/Qt 通用应用基础平台

---

## 1. 四层架构

```
                    SoulCoreKit
                         │
        ┌────────────────┼────────────────┐
        │                │                │
      Core           Infrastructure     Extensions
   (极稳定)         (CS/BS 共用)       (企业级扩展)
        │                │                │
   DI/Error/Event    Network/Data       RPC/MQ/Auth
   Logging/Async     Cache/Storage      Plugin/Scheduler
   Application       Config/Obs         Server/AOP
                     Validation
                         │
                    Application
                    (场景适配)
                         │
                  ┌──────┴──────┐
                  │             │
                 CS            BS
            Qt Client/     Web Backend
            Server
```

---

## 2. 依赖规则 (强制)

```
Application → Extensions → Infrastructure → Core
```

| 层 | 允许依赖 |
|----|----------|
| **Core** | 仅 Qt + 第三方库 (spdlog, nlohmann_json) |
| **Infrastructure** | Core |
| **Extensions** | Core + Infrastructure |
| **Application** | Core + Infrastructure + Extensions |

**编译期检查**: `cmake/SoulCoreKitArchitecture.cmake` 的 `sc_check_architecture()` 在构建时验证所有依赖方向，违规项触发 FATAL_ERROR。

---

## 3. Layer 0: Core — CS/BS 共用极稳定核心

零业务绑定，API 极简稳定。

| 模块 | 职责 | 关键类 |
|------|------|--------|
| `soul_core` | 核心类型与基础设施 | `Result<T>`/`Error`/`Singleton`/`Scaffold`/`Application`/`Environment`/`Platform`/`UUID` |
| `soul_di` | 依赖注入容器 | `Container`/Singleton/Scoped/Transient |
| `soul_logging` | 日志系统 (spdlog) | `Logger`/`ISink`/`ConsoleSink`/`FileSink`/`DailyFileSink` |
| `soul_async` | 异步任务框架 | `ThreadPool`/`TaskRunner`/`Future<T>`/`Promise<T>`/`Coroutine` |
| `soul_event` | 事件总线 | `EventBus`/`TypedEventBus<T>`/`QtSignalAdapter` |
| `soul_application` | 应用上下文 | `ApplicationContext`/`ControllerRegistry`/`ServiceRegistry` |

### 统一生命周期 (ILifecycle)

```
Construct
    ↓
initialize()  → Result<void>  资源分配、DI 绑定
    ↓
start()       → Result<void>  启动服务、监听端口
    ↓
Running                       正常运行
    ↓
stop()        → void          停止接收请求 (保证执行)
    ↓
shutdown()    → void          释放所有资源 (保证执行)
    ↓
Destroy
```

---

## 4. Layer 1: Infrastructure — CS/BS 共用基础设施

### 4.1 通信基础设施 (核心竞争力)

```
       SoulCoreKit
            │
    Network Abstraction
            │
 ┌──────────┼──────────┐
 ↓          ↓          ↓
TCP       HTTP     WebSocket
 │          │          │
CS       BS Backend  CS/BS
```

| 模块 | 职责 |
|------|------|
| `soul_network_core` | NetworkAdapterBase/CodecFactory/Monitor |
| `soul_network_policy` | CircuitBreaker/RetryPolicy/RateLimiter/HeartbeatPolicy |
| `soul_network_http` | HttpClientAdapter/拦截器链 |
| `soul_network_protocol` | TCP/WebSocket/MQTT/Bluetooth/Serial/NamedPipe Adapter |
| `soul_network` | HttpClient/WebSocket/TcpClient/ConnectionPool (聚合) |

### 4.2 数据基础设施

| 模块 | 职责 | 数据库定位 |
|------|------|-----------|
| `soul_data` | IRepository/QueryBuilder/Transaction | 抽象层 |
| `soul_database` | IDatabaseDriver/ConnectionPool | **MySQL** (BS Server 一等公民) |
| `soul_orm` | QueryWrapper/SqlDialect/MigrationManager | **SQLite** (CS Client 本地) |
| `soul_storage` | Memory/File/SQLite KV 存储 | 轻量持久化 |
| `soul_cache` | Memory/Disk/MultiLevel 缓存 | 性能加速 |

### 4.3 通用基础设施

| 模块 | 职责 |
|------|------|
| `soul_base` | BaseObject/BaseManager/BaseService |
| `soul_utils` | Crypto/File/JSON/String/XML/Process 等 10+ 工具类 |
| `soul_configuration` | JSON/INI + 热加载 + Profile 隔离 + ConfigCenter |
| `soul_observability` | Metrics/Tracing/HealthCheck/Prometheus/Otlp |
| `soul_validation` | Validator/注解式校验 |

---

## 5. Layer 2: Extensions — 企业级扩展

保留抽象，不污染核心架构。Adapter 按需编译接入。

| 模块 | 职责 | Adapter |
|------|------|---------|
| `soul_rpc` | RpcClient/RpcServer/Transport 抽象 | HTTP/WS/TCP Transport → gRPC Adapter |
| `soul_mq` | MessageBus + InMemory 核心 | RabbitMQ/Kafka/RocketMQ Adapter |
| `soul_auth` | AuthManager/OAuth2/OIDC | — |
| `soul_plugin` | C-ABI/PluginManager/DLL 加载 | — |
| `soul_scheduler` | ScheduledTask/CronTrigger | — |
| `soul_server` | HttpServer/WebSocketServer (BS 入口) | — |
| `soul_aop` | AspectWeaver/Before/After/Around | — |

---

## 6. Layer 3: Application — 场景适配

| 模块 | 场景 | 职责 |
|------|------|------|
| `soul_ui` | CS Client | 30+ Widgets/Theme/Style/玻璃效果 |
| `soul_cs` | CS 全场景 | CsController/CsRouter/CsViewModel/CsAdminPanel |

---

## 7. 聚合库策略

| 聚合库 | 适用场景 | 包含 |
|--------|----------|------|
| `SoulCoreKit` | BS Backend / CLI / Headless | Core + Infrastructure + Extensions |
| `SoulCoreKitUi` | CS Client (含 UI) | SoulCoreKit + soul_ui |
| `SoulCoreKitFull` | 完整 CS 应用 | SoulCoreKit + SoulCoreKitUi + soul_cs |

---

## 8. 线程模型

详见 [Threading Model](./architecture/threading-model.md)。

7 条强制规则，组件线程安全三级分类 (安全/亲和/需同步)。

---

## 9. Error/Result 模型

```
Error
├── Code        (ErrorCode 枚举)
├── Category    (Resource/Network/Database/...)
├── Message     (QString)
├── Cause       (shared_ptr<Error> 链)
├── Context     (QHash<QString,QVariant> 诊断元数据)
└── Metadata    (预留)

Result<T>
├── isOk()/isErr()
├── unwrap()/expect()
├── map()/andThen()/orElse()  (Monadic)
└── [[nodiscard]] 工厂函数
```

---

## 9. 版本路线图

| 版本 | 主题 | 核心目标 |
|------|------|----------|
| v2.5.x | CS+BS 定位 | 四层架构落地 + README/CMake 对齐 |
| **v2.6.x** | **Foundation Stabilization** | **生命周期/线程/Error/依赖治理** |
| v2.7.x | CS/BS 共用能力 | HTTP/WebSocket/Middleware/RPC |
| v2.8.x | 工程化 + 可观测性 | Health/Benchmark/Performance |
| v2.9.x | 企业能力扩展 | Auth/Config/MQ/Discovery |
| v3.0.0 | API/ABI Freeze | 架构稳定版 |
