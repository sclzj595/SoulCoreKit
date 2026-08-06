# 03 — 模块清单

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 3.1 Foundation 层 (22 个模块)

### 3.1.1 核心基础设施

| 序号 | 模块 | 库名 | 职责 | 关键组件 |
|------|------|------|------|----------|
| 1 | Core | `soul_core` | 核心类型与框架入口 | `Result<T>`, `Error`, `Singleton`, `Application`, `Scaffold`, `Module`, `ModuleRegistry`, `Version`, `CrashHandler`, `ILifecycleManaged`, `AutoConfiguration`, `Banner`, `StartupLogger`, `FeatureFlagManager` (灰度发布/功能开关) |
| 2 | DI | `soul_di` | 依赖注入容器 | `Container` (Singleton/Scoped/Transient), `createScope()/disposeScope()`, `@Qualifier` 命名绑定, `RegistrationInfo` |
| 3 | Logging | `soul_logging` | 日志系统 | `Logger`, `ConsoleSink`, `FileSink`, `DailyFileSink`, `CompositeSink`, `CallbackSink`, `LogFormatter`, spdlog 集成, `SC_*_FMT` 宏 |
| 4 | Configuration | `soul_configuration` | 配置管理 | `Config`, `JsonConfiguration`, `IniConfiguration`, `ConfigSchema`, `ConfigBind`, `RemoteConfig`, `EtcdSource`, `NacosSource`, `ConfigCenterClient` (Etcd/Nacos 统一客户端) |

### 3.1.2 数据与存储

| 序号 | 模块 | 库名 | 职责 | 关键组件 |
|------|------|------|------|----------|
| 5 | Data | `soul_data` | 数据访问抽象 | `IRepository<T,Id>`, `MemoryRepository`, `QueryCache`, `ORM Reflection`, `QueryBuilder`, `MigrationManager` |
| 6 | Database | `soul_database` | 数据库驱动实现 | `IDatabaseDriver`, `DatabaseDriverBase<T>` (CRTP), `BaseRepository`, `Transaction`, `ConnectionPool`, `MySQLDriver`, `PostgresDriver`, `SQLiteDriver` |
| 7 | Storage | `soul_storage` | 存储抽象 | `IStorage`, `FileStorage`, `MemoryStorage`, `SqliteDatabase`, `Settings`, `Cache`, `JsonSerializer` |
| 8 | Cache | `soul_cache` | 缓存系统 | `ICache`, `MemoryCache`, `DiskCache`, `MultiLevelCache`, `SizeEstimator` |

### 3.1.3 通信与异步

| 序号 | 模块 | 库名 | 职责 | 关键组件 |
|------|------|------|------|----------|
| 9 | Async | `soul_async` | 异步任务 | `ThreadPool`, `TaskRunner`, `Future<T>`, `Promise<T>`, `Dispatcher`, `CancelableTask`, `Coroutine` (C++20) |
| 10 | Event | `soul_event` | 事件总线 | `EventBus`, `TypedEventBus<T>`, `MessageBus`, `Subscription`, `QtSignalAdapter` |
| 11 | Network | `soul_network` | 网络通信 (4 子模块) | 见下方 Network 子模块拆分 |
| 12 | RPC | `soul_rpc` | RPC 框架 | `ClientProxy`, `ServiceDispatcher`, `ServiceRegistry`, `HttpTransport`, `ISerializer`, `JsonSerializer`, `GrpcServer`, `GrpcClient` (Unary/Streaming RPC), `ConsulServiceDiscovery`, `EurekaServiceDiscovery`, `NacosServiceDiscovery`, `WeightedLoadBalancer` |

#### Network 子模块 (4 层)

| 子模块 | 库名 | 职责 |
|--------|------|------|
| Network Core | `soul_network_core` | 网络抽象 (`INetwork`, `IInterceptor`, `INetworkPolicy`, `ICodec`, `ConnectionStateMachine`) |
| Network Policy | `soul_network_policy` | 策略实现 (`RetryPolicy`, `TimeoutPolicy`, `CircuitBreaker`, `RateLimiter`, `HeartbeatPolicy`, `ReconnectPolicy`) |
| Network HTTP | `soul_network_http` | HTTP 协议栈 (`HttpClient`, `ConnectionPool`, `Monitor`, `AuthInterceptor`, `LoggingInterceptor`, `CodecFactory`) |
| Network Protocol | `soul_network_protocol` | 协议适配器 (`WebSocket`, `TCP`, `MQTT`, `Bluetooth`, `Serial`, `NamedPipe`) |

### 3.1.4 业务支撑

| 序号 | 模块 | 库名 | 职责 | 关键组件 |
|------|------|------|------|----------|
| 13 | Auth | `soul_auth` | 认证授权 | `AuthManager`, `TokenManager`, `OAuth2`, `OIDC`, `Permission`, `SecureStorage`, `User` |
| 14 | MQ | `soul_mq` | 消息队列 | `IAmqpBackend`, `InMemoryAmqpBackend`, `AmqpCppBackend` (RabbitMQ), `MQFactory`, `RabbitMQConnection/Producer/Consumer`, `KafkaConnection/Producer/Consumer`, `RocketMQConnection/Producer/Consumer` |
| 15 | ORM | `soul_orm` | 对象关系映射 | `Entity<T>`, `BaseRepository<T>`, `QueryWrapper`, `SqlDialect`, `MigrationManager`, `CachedRepository`, `Reflection`, `CodeGenerator`, `TypedQueryWrapper` |
| 16 | Observability | `soul_observability` | 可观测性 | `Metrics` (Counter/Gauge/Histogram), `PrometheusExporter`, `OtlpHttpExporter` (OpenTelemetry Protocol), `Tracing` (W3C TraceContext), `JsonSink`, `ResourcePoolMonitor` |
| 17 | Base | `soul_base` | 基础类 | `BaseObject`, `BaseManager`, `BaseService` |
| 18 | Utils | `soul_utils` | 工具函数集 | `JsonUtils`, `FileUtils`, `StringUtils`, `CryptoUtils`, `ImageUtils`, `CompressUtils`, `DateTimeUtils`, `ClipboardUtils`, `ProcessUtils`, `XmlUtils` |

### 3.1.5 横切关注点

| 序号 | 模块 | 库名 | 职责 | 关键组件 |
|------|------|------|------|----------|
| 19 | Validation | `soul_validation` | 输入验证 | `Validator`, `ValidationError`, `ValidationResult` |
| 20 | AOP | `soul_aop` | 切面编程 | `JoinPoint`, `Pointcut`, `Advice` (5 种), `Aspect`, `AspectWeaver` |
| 21 | Scheduler | `soul_scheduler` | 定时任务 | `Scheduler`, `ScheduledTask` |
| 22 | Server | `soul_server` | 嵌入式 HTTP Server | `HttpServer`, `HealthEndpoint`, `InfoEndpoint`, `MetricsEndpoint`, `BeansEndpoint`, `CachesEndpoint`, `LoggersEndpoint`, `MappingsEndpoint`, `ScheduledTasksEndpoint`, `ShutdownEndpoint`, `ThreadDumpEndpoint`, `EnvEndpoint`, `Middleware`, `WebSocketServer` |
| 23 | Plugin | `soul_plugin` | 插件系统 | `IPlugin`, `PluginManager`, `PluginHandle` |

---

## 3.2 Application 层 (3 个模块)

| 序号 | 模块 | 库名 | 职责 | 关键组件 |
|------|------|------|------|----------|
| 24 | Application | `soul_application` | 应用上下文 (INTERFACE) | `ApplicationContext`, `ServiceRegistry`, `ControllerRegistry` |
| 25 | CS | `soul_cs` | CS 架构核心 | `CsRouter`, `CsController`, `CsService`, `CsViewModel`, `CsErrorHandler`, `CsModule`, `CsDataBinding`, `CsNavigation`, `CsDialogManager`, `CsWindowManager`, `CsFormValidator`, `CsAdminPanel` (管理后台面板), `CsIpcRouter` (进程间通信路由) |
| 26 | UI | `soul_ui` | UI 组件库 (30+ 组件) | 见 [05_ui_components.md](05_ui_components.md) |

---

## 3.3 聚合库

| 库名 | 类型 | 包含模块 |
|------|------|----------|
| `SoulCoreKit` | INTERFACE | 所有 Foundation + Application 模块（不含 UI） |
| `SoulCoreKitUi` | INTERFACE | SoulCoreKit + soul_ui |

---

## 3.4 模块依赖规则

1. **Foundation 层**不依赖 Application 层
2. **Core** 模块零内部依赖（仅依赖 Qt::Core）
3. **Application 层**依赖 Foundation 层
4. **禁止循环依赖**
5. **单向依赖流**: `View → ViewModel → Controller → Service → Repository`