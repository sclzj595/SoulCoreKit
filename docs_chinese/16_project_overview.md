# SoulCoreKit 项目全貌说明

> **版本**: v3.0.0  
> **语言**: C++17 + Qt 6.5.3  
> **构建**: CMake 3.31 + MinGW/MSVC  
> **状态**: API / Architecture Freeze  
> **文档生成**: 2026-08-09

---

## 1. 项目定位

SoulCoreKit 是一个 **面向 CS + BS 双场景的 C++/Qt 通用应用基础平台**，灵感来自 Spring Boot / ASP.NET Core 的模块化设计思想。

不再是单纯的 Qt CS 工具库，而是让开发者用同一套框架同时构建：
- **CS Qt Desktop Client**（scChat 等桌面应用）
- **CS Qt Server**（SoulCove 等后台服务）
- **BS Web Backend**（HTTP REST API / WebSocket Server）
- **CLI / Headless 工具**（命令行工具、守护进程）

```
                    SoulCoreKit
                         │
          ┌──────────────┴──────────────┐
          │                             │
         CS                            BS
          │                             │
    Qt Client/Server              Web Backend
          │                             │
    ┌─────┼─────┐              ┌────────┼────────┐
    │     │     │              │        │        │
  Desktop  LAN   IPC        HTTP/WS    REST     RPC
  Client   App   App        Server    API      Backend
          │                             │
          └──────────────┬──────────────┘
                         │
                    Shared Core
```

---

## 2. 四层架构 (Architecture Invariant, v3.0.0 Frozen)

```
                    SoulCoreKit
                         │
        ┌────────────────┼────────────────┐
        │                │                │
      Core           Infrastructure     Extensions      Application
   (极稳定)         (CS/BS共用)        (企业级扩展)    (场景适配)
        │                │                │              │
   DI/Error/Event    Network/Data       RPC/MQ/Auth     UI/CS
   Logging/Async     Cache/Storage      Plugin/Scheduler
   Lifecycle         Config/Obs         Server/AOP
                     Validation
```

### Layer 0: Core — CS/BS 共用极稳定核心

5 个模块，零业务绑定：

| 模块 | 职责 |
|------|------|
| `soul_core` | `Result<T>`/`Error`/`Application`/`Environment`/`Platform`/`UUID`/`Lifecycle`/`Health` |
| `soul_di` | DI Container: Singleton/Scoped/Transient |
| `soul_logging` | 日志: spdlog 后端,多 Sink |
| `soul_async` | 异步: ThreadPool/Future/Promise |
| `soul_event` | 事件总线: EventBus/TypedEventBus/MessageBus |

### Layer 1: Infrastructure — CS/BS 共用基础设施

15 个模块，统一抽象、多后端适配：

**通信基础设施**:
| 模块 | 职责 |
|------|------|
| `soul_network_core` | NetworkAdapterBase/CodecFactory/Monitor |
| `soul_network_policy` | CircuitBreaker/RetryPolicy/RateLimiter/HeartbeatPolicy |
| `soul_network_http` | HttpClientAdapter/拦截器链 |
| `soul_network_protocol` | TCP/WebSocket/MQTT/Bluetooth/Serial/NamedPipe Adapter |
| `soul_network` | HttpClient/WebSocket/TcpClient/ConnectionPool (聚合) |

**数据基础设施**:
| 模块 | 职责 |
|------|------|
| `soul_data` | IRepository/QueryBuilder/Transaction/ORM Reflection/Migration |
| `soul_database` | MySQL(BS Server) / SQLite(CS Client) |
| `soul_orm` | QueryWrapper/SqlDialect/MigrationManager/CachedRepository |
| `soul_storage` | Memory/File/SQLite/Settings KV存储 |
| `soul_cache` | ICache/MemoryCache/DiskCache/MultiLevelCache/RedisCache (canonical) |

**通用基础设施**:
| 模块 | 职责 |
|------|------|
| `soul_application` | 应用上下文: ApplicationContext/ControllerRegistry/ServiceRegistry |
| `soul_base` | BaseObject/BaseManager/BaseService |
| `soul_utils` | Crypto/File/JSON/String/XML/Process 等 10+ 工具类 |
| `soul_configuration` | PriorityConfigChain/ConfigSnapshot/IConfigProvider + 5 Providers (canonical) |
| `soul_observability` | Metrics/Tracing/HealthCheck/Prometheus/Otlp |
| `soul_validation` | Validator/声明式校验 |

### Layer 2: Extensions — 企业级扩展

7 个模块，保留抽象，不污染核心架构：

| 模块 | 职责 |
|------|------|
| `soul_rpc` | RpcClient/RpcServer/ServiceDispatcher/WeightedLoadBalancer/ServiceDiscovery |
| `soul_mq` | MessageBus + InMemory核心 + RabbitMQ/Kafka/RocketMQ Adapter |
| `soul_auth` | AuthManager/TokenManager/OAuth2/OIDC |
| `soul_plugin` | C-ABI/PluginManager/DLL动态加载 |
| `soul_scheduler` | ScheduledTask/CronTrigger |
| `soul_server` | HttpServer/WebSocketServer/Middleware — BS场景核心入口 |
| `soul_aop` | AspectWeaver/Before/After/Around |

### Layer 3: Application — 业务场景适配

2 个模块，按场景按需链接：

| 模块 | 场景 | 职责 |
|------|------|------|
| `soul_ui` | CS Client | 30+ Widgets/Theme/Style/玻璃效果 |
| `soul_cs` | CS 全场景 | CsController/CsRouter/CsViewModel/CsAdminPanel |

---

## 3. 聚合库策略

| 聚合库 | 适用场景 | 包含 |
|--------|----------|------|
| `SoulCoreKit` | BS Backend / CLI / Headless / CS Server | Core + Infrastructure + Extensions |
| `SoulCoreKitUi` | CS Client (含 UI) | SoulCoreKit + soul_ui |
| `SoulCoreKitFull` | 完整 CS 应用 | SoulCoreKit + SoulCoreKitUi + soul_cs |

---

## 4. v3.0.0 Freeze 状态

### 已冻结的契约

| 契约 | 状态 | 证据 |
|------|:---:|------|
| 四层架构 | ✅ Frozen | `sc_check_architecture()` 编译期强制 |
| Lifecycle Contract | ✅ Frozen | ILifecycle 四阶段 + shutdown 幂等 |
| Thread Safety Model | ✅ Frozen | A/B/C/D 四级分类 + threading-model.md |
| Extension Contract | ✅ Frozen | 9 项强制契约 + extension-contract.md |
| CMake Targets | ✅ Frozen | 30 模块 + 3 聚合库 |
| Configuration (canonical) | ✅ Unified | PriorityConfigChain + ConfigSnapshot |
| Cache (canonical) | ✅ Unified | soul/cache/ ICache/MemoryCache/DiskCache/RedisCache |
| LoadBalancer (canonical) | ✅ Unified | WeightedLoadBalancer (4 策略) |

### v3.0.0 Legacy API Removal (已完成)

| Legacy API | Canonical | 状态 |
|-----------|-----------|:---:|
| `sc::Configuration` | `PriorityConfigChain` + `ConfigSnapshot` | ✅ Removed |
| `soul/storage/cache.h` | `soul/cache/` | ✅ Removed |
| `sc::rpc::LoadBalancer` | `sc::rpc::WeightedLoadBalancer` | ✅ Removed |
| `ILifecycleManaged` | `ILifecycle` | ✅ Removed |
| `getServiceNamesCompat()` | `getServiceNames()` | ✅ Removed |

---

## 5. 核心特性

### Configuration — PriorityConfigChain + ConfigSnapshot (v3.0 canonical)

```cpp
auto chain = std::make_shared<PriorityConfigChain>();
chain->addProvider(std::make_shared<DefaultConfigProvider>());
chain->addProvider(std::make_shared<JsonFileConfigProvider>("app.json"));
chain->addProvider(std::make_shared<EnvironmentConfigProvider>("APP_"));

auto result = chain->load(); // → ConfigSnapshot (不可变)
if (auto snapshot = result.unwrap()) {
    int port = snapshot.getIntOr("server.port", 8080);
}
```

### Cache — sc::cache::ICache (v3.0 canonical)

```cpp
sc::cache::MemoryCache<std::string, User> cache({.maxEntries = 10000});
auto user = cache.get("user:42"); // → Result<optional<User>>
cache.put("user:42", user, std::chrono::minutes(5));
```

### Error/Result 模型

```cpp
Result<User> user = repo.findById(42);
auto name = user.map([](auto& u) { return u.name; })
                .orElse("unknown");
```
- **无异常设计**：所有错误通过 `Result<T>` 返回值传播
- **ErrorCode 枚举**：NotFound / Timeout / InternalError / InvalidArgument 等

### DI 容器

```cpp
class MyModule : public Module {
    void configure() override {
        bind<IUserRepo, SqlUserRepo>().in<SingletonScope>();
        bind<AuthService>().in<TransientScope>();
    }
};
```
- **三种生命周期**：Transient / Singleton / Scoped

### 统一生命周期 (ILifecycle)

```
Construct → initialize() → start() → Running → stop() → shutdown() → Destroy
```

### ServiceDiscovery + WeightedLoadBalancer

```
IServiceDiscovery → WeightedLoadBalancer
  ├── RoundRobin
  ├── WeightedRoundRobin
  ├── LeastConnections
  └── Random
```

### Observability

- Metrics: Counter/Gauge/Histogram + Prometheus Exporter
- Tracing: TraceContext/Span/Tracer + OtlpExporter
- Health: IHealthIndicator + HealthAggregator + Liveness/Readiness

---

## 6. 版本历史 (关键里程碑)

| 版本 | 主题 |
|------|------|
| v1.0.0 | 初始发布 |
| v1.9.4 | Actuator 端点 + 运维增强 |
| v2.0.0 | Application 启动器 + ORM + Migration |
| v2.5.0 | CS 架构定稿 + 30 模块体系 |
| v2.6.0 | 四层架构正式落地 + 生命周期统一 + 线程模型冻结 |
| v2.7.0 | Middleware 体系 + RESTful 路由 + RPC Transport 抽象 |
| v2.8.0 | Observability + Health + Benchmark 体系 |
| v2.9.0 | Configuration 统一抽象 + Enterprise Extension 边界 |
| v2.9.1 | MessageBus + Event Messaging |
| v2.9.2 | Cache/Redis + Subscription RAII |
| v2.9.3 | Service Discovery + Extension Contract |
| v2.9.4 | Architecture Consolidation — 双轨收敛 + 治理体系 |
| **v3.0.0** | **API / Architecture Freeze — Legacy API Removal + Contract Frozen** |

---

## 7. 快速开始

```bash
# 配置 (MinGW)
cmake -S . -B build -G "MinGW Makefiles" \
  -DCMAKE_CXX_COMPILER=F:/IDE.2/QT/Tools/mingw1120_64/bin/g++.exe \
  -DQT6_DIR=F:/IDE.2/QT/6.5.3/mingw_64 \
  -DBUILD_TESTS=ON

# 编译
cmake --build build -j4

# 运行测试
cd build && ctest --output-on-failure
```

---

*SoulCoreKit v3.0.0 — API / Architecture Freeze*
