# SoulCoreKit Architecture Consolidation v3.0.0

> **状态**: API / Architecture Freeze  
> **目标**: 收敛 v2.5.x–v2.9.4 期间产生的双轨抽象、生命周期差异和依赖债务  
> **下一个里程碑**: v3.x 稳定演进 (Bug Fix / Backward-compatible Features / Performance Regression)
> 
> **v3.0.0 双轨收敛**: 全部完成 (3 项 legacy API 已从代码/CMake/测试中移除，canonical API 为唯一实现)
> **ABI Baseline**: 参见 Freeze Gate Report (未在本文档中声明 Freeze)

---

## 1. 双轨收敛

### 1.1 Configuration 双轨 → 单轨

| 项目 | Track A (旧) | Track B (新) | 状态 |
|------|-------------|-------------|------|
| 类 | `sc::Configuration` | `sc::Config` + `IConfigProvider` | |
| 头文件 | `soul/core/configuration.h` | `soul/configuration/config.h` | |
| 使用位置 | `Application::loadConfiguration()` | `ui/theme.cpp` | |
| 功能 | YAML, Profile | JSON/INI/Env/Remote, 热加载, Schema | |
| **v2.9.4 处理** | **deprecated** | **canonical** | ✅ |
| **v3.0 处理** | **移除 (header+source 已删除)** | **Application 迁移到 PriorityConfigChain+ConfigSnapshot** | ✅ |

### 1.2 Cache 双轨 → 单轨

| 项目 | Track A (旧) | Track B (新) | 状态 |
|------|-------------|-------------|------|
| 头文件 | `soul/storage/cache.h` | `soul/cache/icache.h` | |
| 命名空间 | `sc::` | `sc::cache::` | |
| get() 签名 | `Result<V>` (NotFound=Err) | `Result<optional<V>>` (Miss≠Error) | |
| 生产代码 | 测试/基准 | ORM `cached_repository.h` | |
| **v2.9.4 处理** | **deprecated** | **canonical** | ✅ |
| **v3.0 处理** | **移除 (header+source 已删除, test 已迁移)** | **唯一实现 (soul/cache/)** | ✅ |

### 1.3 LoadBalancer 统一

| 项目 | 旧 | 新 | 状态 |
|------|---|----|------|
| 类 | `LoadBalancer` (2策略) | `WeightedLoadBalancer` (4策略) | |
| 头文件 | `service_registry.h` | `service_discovery.h` | |
| **v2.9.4 处理** | **deprecated** | **canonical** | ✅ |
| **v3.0 处理** | **移除 (class declaration+impl 已删除, test_rpc 已迁移到 WeightedLoadBalancer)** | **唯一实现** | ✅ |

---

## 2. 生命周期契约

所有公共模块必须遵守统一生命周期：

```
Construct → initialize() → start() → Running → stop() → shutdown() → Destroy
```

### 验收矩阵

| 模块 | initialize | start | stop | shutdown | 幂等 | 异常安全 |
|------|:---:|:---:|:---:|:---:|:---:|:---:|
| Configuration | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| MessageBus | N/A | N/A | N/A | ✅ | ✅ | ✅ |
| Cache | N/A | N/A | N/A | N/A | N/A | ✅ |
| Discovery | N/A | N/A | N/A | ✅ | ✅ | ✅ |
| HttpServer | N/A | N/A | ✅ | ✅ | ✅ | ✅ |
| RPC | ✅ | ✅ | ✅ | ✅ | ✅ | ✅ |
| Health | N/A | N/A | N/A | N/A | N/A | ✅ |

### ApplicationState vs LifecycleState

| 枚举 | 状态数 | 用途 | 状态 |
|------|--------|------|------|
| `LifecycleState` | 10 | 通用组件生命周期 | canonical |
| `ApplicationState` | 5 | Application 专属 | 保留独立 |
| **v3.0 处理** | ApplicationState 保留独立语义 (Application 级状态), LifecycleState 为通用组件生命周期 canonical | ✅ 明确分离 |

---

## 3. 线程安全分类

所有公共类型必须标注线程安全级别：

| 级别 | 含义 | 示例 |
|------|------|------|
| **A** | Immutable / Thread-safe (无锁) | `ConfigSnapshot`, `ServiceInstance`, `Result<T>`, `Error` |
| **B** | Internal synchronization (有锁) | `InMemoryMessageBus`, `InMemoryServiceRegistry`, `HealthAggregator` |
| **C** | Thread-confined (必须单线程使用) | `QWidget`, `QSqlDatabase connection` |
| **D** | External synchronization required | `Configuration` (write path) |

### 模块分类

| 模块 | 核心类型 | 线程安全级别 |
|------|----------|:---:|
| Core | `Result<T>`, `Error`, `Uuid` | A |
| DI | `Container::resolve()` | B |
| Event | `InMemoryMessageBus` | B |
| Configuration | `ConfigSnapshot` | A |
| Configuration | `PriorityConfigChain::load()` | B |
| Cache | `MemoryCache`, `DiskCache` | B |
| Discovery | `InMemoryServiceDiscovery` | B |
| Health | `HealthAggregator` | B |

---

## 4. 所有权矩阵

| 对象 | Owner | 生命周期 | 指针类型 |
|------|-------|----------|----------|
| `IMessageBus` | Application | Application | `shared_ptr` |
| `MessageSubscription` | Caller | Scope | `shared_ptr` (Bus 存 `weak_ptr`) |
| `ICache` | Application | Application | `shared_ptr` |
| `IServiceDiscovery` | Application | Application | `shared_ptr` |
| `HealthAggregator` | Singleton | Process | Singleton |
| `Module*` (use) | Application (不拥有) | 外部 | `raw*` (明确标注) |
| `Module*` (scan) | Application (拥有) | 内部 | `unique_ptr` |
| `IHealthIndicator` | Aggregator (不拥有) | Provider lifetime | `shared_ptr` |

---

## 5. 依赖图验证

```
Application  → Extensions → Infrastructure → Core
```

### v2.9.4 验证结果

| 检查项 | 结果 |
|--------|:---:|
| Core → Infrastructure | ❌ 无违规 |
| Core → Extensions | ❌ 无违规 |
| Infrastructure → Extensions | ❌ 无违规 |
| Infrastructure → Application | ❌ 无违规 |
| Extensions → Application | ❌ 无违规 |
| Infrastructure 内部重复 | ⚠️ storage/cache vs cache/ (已处理) |
| CMake 架构检查 | ✅ `sc_check_architecture()` 通过 |

---

## 6. Legacy API v3.0 Removal List

以下 API 已在 v3.0.0 移除：

| API | 替代 | 状态 |
|-----|------|:---:|
| `sc::Configuration` (core) | `sc::Config` + `IConfigProvider` | ✅ 已移除 |
| `soul/storage/cache.h` | `soul/cache/` | ✅ 已移除 |
| `sc::rpc::LoadBalancer` | `sc::rpc::WeightedLoadBalancer` | ✅ 已移除 |
| `Application::loadConfiguration()` (旧实现) | 迁移到新 Configuration | ✅ 已迁移 |
| `ILifecycleManaged` (deprecated alias) | `ILifecycle` | ✅ 已移除 |
| `getServiceNamesCompat()` | `getServiceNames()` | ✅ 已移除 |

---

## 7. v3.0.0 最终变更清单

| 变更 | 文件 | 操作 |
|------|------|------|
| Cache 双轨 deprecated | `soul/storage/cache.h` | v2.9.4: +deprecated 注释 |
| Cache 双轨 removal | `soul/storage/cache.h` + `src/soul/storage/cache.cpp` | v3.0.0: **已删除** |
| Cache 聚合头更新 | `soul/soul_storage.h` | v2.9.4: +Track B includes (canonical) |
| Configuration deprecated | `soul/core/configuration.h` | v2.9.4: +deprecated 注释 |
| Configuration removal | `soul/core/configuration.h` + `src/soul/core/configuration.cpp` | v3.0.0: **已删除** |
| LoadBalancer deprecated | `soul/rpc/service_registry.h` | v2.9.3: 已有 |
| LoadBalancer removal | class declaration + impl + test migration | v3.0.0: **已删除, test 迁移到 WeightedLoadBalancer** |
| 架构收敛文档 | `docs/architecture/architecture-consolidation.md` | v3.0.0: 最终状态同步 |
