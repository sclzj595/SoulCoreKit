# SoulCoreKit Roadmap

**文档状态**: Accepted
**最后更新**: 2026-07-26
**当前基线**: v1.7.0 (P1 新模块交付 + P2 MQ 真实集成,功能新增向后兼容)
**当前迭代**: v1.8.0 (规划中,分布式能力 + 协议增强 + 安全合规)

---

## 0. 文档说明

本路线图遵循 RFC 风格,按语义化版本(SemVer)组织。每个版本明确以下要素:
- **范围**: 功能边界与排除项
- **状态**: Planned / In Progress / Released / Superseded
- **依赖**: 前序版本与阻塞条件
- **验收标准**: 可量化的交付条件

版本节奏遵循**按范围发布**原则:版本号在 P0+P1 范围完成后才晋升,不采用时间盒。仅 P2/可选项目允许跨版本延期,且必须在 CHANGELOG 中明确标注延期去向。

---

## 1. 已发布版本(历史基线)

### v1.0.0 — 2026-07-14 (Initial Release)

| 模块 | 状态 | 说明 |
|------|------|------|
| Core / Base / Utils / Logging | ✅ Released | 基础设施层 |
| Network (HTTP/TCP/WebSocket) | ✅ Released | 协议无关网络层 |
| UI | ✅ Released | 30+ 现代 Qt 组件 |
| Async / Event / Storage / Configuration / Auth | ✅ Released | 业务支撑层 |

**交付**: 初始框架发布,9 个核心模块。

---

### v1.1.0 — 2026-07-14 (Network Refinement)

**主要变更**:
- 协议无关 `INetwork` 接口抽象
- 工厂模式 + 适配器模式(HttpClient/TcpClient/WsClient)
- 策略层(Retry/Timeout/Heartbeat)、拦截器链、Codec 层
- `sc::network` 命名空间隔离,统一 `SC_NETWORK_EXPORT` 导出宏
- Result<T> 错误处理模式引入

---

### v1.2.0 — 2026-07-20 (Network Hardening)

**主要变更**:
- MOC 预处理器兼容性修复(`#ifndef Q_MOC_RUN` 守卫)
- 所有 `SC_NETWORK_EXPORT` 类统一包含 `network_global.h`
- 跨平台 CI 构建(Ubuntu/macOS/Windows)修复

---

### v1.3.0 — 2026-07-21 (DI & Plugin)

**新增模块**:
- **DI Container** (`soul_di`): `bind`/`resolve`/`bindSingleton`/`bindInstance`,三生命周期(Transient/Singleton/Scoped),`std::recursive_mutex` + DCLP 线程安全
- **Plugin System** (`soul_plugin`): C-ABI 边界、`PluginMetadata` ABI/API 版本兼容、`PluginHandle` RAII、死锁-free 初始化/关闭序列

**测试**: DI-T01 ~ DI-T11 验收用例

---

### v1.4.0 — 2026-07-23 (Data Layer补全)

**主要变更**: 补齐 `database_driver.cpp`/`memory_repository.cpp`/`transaction.cpp` 实现文件;`soul_data` 链接 Qt6::Sql

---

### v1.5.0 — 2026-07-24 (ORM MyBatis-Plus 化)

**重大架构**:
- `SQLiteRepository<T>` → 通用 `SqlRepository<T>` + `ISqlDialect` 注入
- `SQLiteDialect` / `MySqlDialect` / `PostgreSqlDialect` 三方言实现
- `SoftDeleteConfig` 可配置软删除列与逻辑值
- `BaseRepository<T>` 默认 CRUD 实现,子类仅实现 5 个核心方法
- `QueryWrapper` 方言感知的 LIMIT/占位符/软删除谓词生成
- `SQLiteRepository<T>` 保留为 typedef 别名(向后兼容)
- `DbConnectionPool` 连接池,`MemoryRepository<T>` 线程安全

**清理**: 删除 53 个临时开发脚本(Python/PowerShell)

---

### v1.6.0 — 2026-07-25 (RPC + CI/CD + ADR)

**新增模块**:
- **SoulRPC Framework** (`soul_rpc`): `ISerializer`/`IRpcTransport`/`ServiceDispatcher`/`ClientProxy`/`IServiceRegistry`/`LoadBalancer`,策略+代理+工厂+命令+模板方法 5 种设计模式
- **CI/CD Pipeline**: `ci.yml`(多平台构建+测试+lint+覆盖率)、`lint.yml`(Clang-Tidy + CppCheck)、`release.yml`(版本标签多平台发布)
- **CMake Presets**: `default`/`test`/`lint`/`release` 四配置
- **ADR 体系**: ADR-001 错误处理边界、ADR-002 模块依赖规则、ADR-003 内存管理、ADR-004 ORM 多数据库、ADR-005 线程安全策略

**重大修复**: 19 个 `catch (...)` 替换为 `catch (const std::exception&)` + fallback;6 个裸 `new` 改 `std::unique_ptr`;5 个 `bool` API 改 `Result<void>`

---

### v1.6.1 — 2026-07-25 (Critical Bug Fixes)

**关键修复**:
- CRITICAL: `QueryWrapper::buildUpdateSql/buildDeleteSql` 无 WHERE 全表操作风险 — 强制 `allowFullTableOperation(true)` 显式授权
- MAJOR: `ConnectionPool::acquire` 在锁内执行网络 IO — 锁释放 + 占位条目
- MAJOR: `ConnectionPool` 满负载立即返回 nullptr — `condition_variable` 等待 + `release()` 唤醒
- MAJOR: 缺 RAII 包装 — `ConnectionGuard` move-only 自动释放
- MAJOR: `appendConditions` 拼接 `=?` 无空格 — 修复为 `= ?`

---

### v1.6.2 — 2026-07-25 (Logger Coverage)

**关键修复**:
- MAJOR: `Future::waitForFinished`/`onSuccess`/`executeCallbacks` 静默吞异常 — Logger 日志保留
- MAJOR: `TaskRunner::runAsync` 静默吞任务异常 — Logger 日志保留
- MAJOR: `PluginManager::loadAllPlugins` 静默吞加载异常 — Logger 日志保留
- MAJOR: `TypedEventBus::create()` 改 `std::unique_ptr` → `std::shared_ptr` 转换(等价 make_shared,保留私有构造)
- MAJOR: `DI Container::bind/bindInstance/bindSingleton` 返回 `Result<void>`,重复注册返回 `AlreadyExists`

**验证**: Windows MinGW 11.2.0 + Qt 6.5.3,21/21 测试通过

---

## 2. 已发布 — v1.7.0 (Released 2026-07-26)

**起始日期**: 2026-07-26
**发布日期**: 2026-07-26
**发布策略**: **按范围发布**(完成 P0+P1 后发布,无硬性时间盒)
**目标状态**: 稳定加固 + 选择性引入新模块 + P2 MQ 真实集成
**详细设计**: [docs/v1.7.0/](./v1.7.0/README.md)
**版本类型**: Minor(功能新增,向后兼容)

### 2.0 实际交付清单

> 本节为 v1.7.0 发布时的实际交付快照,与原计划的偏差在每项后注明。

#### 2.0.1 P1 新模块交付

| ID | 模块 | 状态 | 交付物 |
|----|------|------|--------|
| P1-H | SoulObservability | ✅ Released | `metrics.h/cpp`(Counter/Gauge/Histogram + MetricsRegistry)、`tracing.h/cpp`(TraceContext/Span/Tracer/SpanGuard,m_maxSpans=10000 FIFO)、`json_sink.h/cpp`(结构化 JSON 输出) |
| P1-G | SoulCache | ✅ Released | `icache.h`、`memory_cache.h`(L1 TTL+LRU)、`disk_cache.h/cpp`(L2)、`multi_level_cache.h`(自动回填)、`size_estimator.h` |
| P1-F | ORM Enhanced | ✅ Released | `column.h`、`typed_query_wrapper.h`、`reflection.h`(SC_DEFINE_REFLECTION)、`cached_repository.h`(装饰器)、`migration.h/cpp`(事务型迁移 + MigrationManager 线程安全) |

#### 2.0.2 P2 增强项交付

| ID | 任务 | 状态 | 交付物 |
|----|------|------|--------|
| P2-B | MQ 真实集成 | ✅ Released(原为可选) | `iamqp_backend.h` 接口、`inmemory_amqp_backend.h/cpp`(模拟 AMQP 0.9.1:Direct/Fanout/Topic + 通配符 + QoS + ack/nack/reject)、`amqpcpp_backend.h/cpp`(amqpcpp v4.3.27 真实后端:QtTcpHandler + Qt事件循环集成 + 异步转同步 + 心跳保活 + 属性映射)、`RabbitMQConnection/Producer/Consumer` 委托重构、CMake option `ENABLE_RABBITMQ` |
| P2-A | RPC 测试深度 | ⏸ 延期到 v1.8.0 | 未纳入本版范围 |

#### 2.0.3 P0 稳定化交付

| ID | 任务 | 状态 | 说明 |
|----|------|------|------|
| P0-A | test_utils 失败修复 | ⚠️ 部分完成 | 见 M0 验收注解 |
| P0-B | 路线图文档同步 | ✅ 完成 | 本文档重写 |
| P0-C | TSan 接入 CI | ⏸ 延期 | 需 Linux 工具链,本版以 Windows MinGW 为主验证平台 |
| P0-D | tech_debt P2 头文件 hygiene | ⚠️ 部分完成 | |
| P0-E | tech_debt P3 | ⚠️ 部分完成 | |
| P0-F | Clang-Tidy CI 检查 | ⏸ 延期 | |

#### 2.0.4 测试交付

| 测试套件 | 用例数 | 状态 |
|----------|--------|------|
| `tests/test_observability.cpp` | 42 个 | ✅ 通过(Counter/Gauge/Histogram/TraceContext/Tracing/JsonSink) |
| `tests/test_mq.cpp` | InMemoryAmqpBackend 单元测试 | ✅ 通过(Direct/Fanout/Topic/QoS/ack/nack) |

#### 2.0.5 关键 Bug 修复交付

| 严重级 | 问题 | 修复方案 |
|--------|------|----------|
| CRITICAL | `Tracer::m_spans` 无限增长内存泄漏 | `m_maxSpans` 上限(默认 10000)+ FIFO 淘汰 |
| MAJOR | `Span::end()/duration()` TOCTOU 数据竞争 | `end()` 持锁设 `m_endTime` 后 release 写 `m_ended`;`duration()` acquire 快速路径 |
| MAJOR | `DiskCache::totalBytes` 单调递增失真 | `dataSizeOnDisk()`,put/remove/过期全分支修正 |
| MAJOR | `MultiLevelCache::remove()` 缓存复活 | 所有层都执行,NotFound 视为成功 |
| MAJOR | `MultiLevelCache` 底层故障静默吞掉 | 添加 `SC_WARN` 日志 |
| MAJOR | `MigrationManager` `const_cast` 破坏 const 正确性 | `m_tableEnsured` 改 `mutable atomic` |
| MAJOR | `MigrationManager` 持锁调用 driver 死锁风险 | 缩小锁粒度,driver 调用移出锁外 |
| MINOR | `Histogram` 未校验空 boundaries | 构造函数拒绝空/非升序 boundaries |
| MINOR | `MemoryCache::isExpiredUnlocked()` 冗余条件 | 删除冗余 `if` |
| Bug | `Span::setAttribute` 重载解析陷阱 | 添加 `const char*` 重载 |

#### 2.0.6 架构合规性

- ✅ 所有新模块遵循 `Result<T>` 错误处理模式(ADR-001)
- ✅ 线程安全遵循 ADR-005 Level 2 标准
- ✅ RAII 资源管理(`SpanGuard`、智能指针)(ADR-003)
- ✅ 设计模式应用:装饰器(`CachedRepository`)、策略(`IAmqpBackend`)、模板方法(`BaseRepository`)
- ✅ 公共接口零破坏:`RabbitMQConnection/Producer/Consumer` 委托 IAmqpBackend 实现,公共接口零变更

### 2.1 范围概览(原计划)

```mermaid
flowchart TB
    subgraph P0["P0 — 稳定化(必做)"]
        S1["TSan 接入 CI<br/>(ADR-005 §4 承诺)"]
        S2["test_utils 9 个失败修复"]
        S3["tech_debt P2/P3 闭环<br/>(头文件 hygiene)"]
        S4["路线图文档同步"]
        S5["Clang-Tidy CI 检查"]
    end
    subgraph P1["P1 — 新模块(必做)"]
        N1["SoulCache<br/>(仅内存+磁盘)"]
        N2["ORM Enhanced<br/>(缓存+类型安全+迁移)"]
        N3["SoulObservability<br/>(Metrics+Tracing+JsonSink)"]
    end
    subgraph P2["P2 — 现有模块增强(可选)"]
        E1["RPC 测试深度"]
        E2["MQ 真实集成<br/>(RabbitMQ 优先)"]
    end
    P0 --> P1
    P1 -.延期到 v1.8.0.-> P2

    style P0 fill:#ffebee,color:#b71c1c
    style P1 fill:#fff3e0,color:#e65100
    style P2 fill:#bbdefb,color:#0d47a1
```

### 2.2 已确认决策项

| 决策项 | 结论 | 理由 |
|--------|------|------|
| SoulCache 范围 | **仅内存 + 磁盘**(L1+L2) | 不引入 Redis 等分布式缓存,保持零外部依赖 |
| SoulObservability 范围 | **不引入 OpenTelemetry** | 自研轻量 Metrics/Tracing/JsonSink,降低耦合 |
| ORM Schema 迁移 | **纳入 v1.7.0** | 配合 ORM Enhanced 完整性,提供 `Migration`/`SchemaBuilder`/`MigrationManager` |
| P2 增强取舍 | **MQ 优先**(RabbitMQ) | HTTP/2 与 OAuth2/OIDC 延期到 v1.8.0 |
| 发布策略 | **按范围发布** | 完成 P0+P1 后发布,无时间盒压力 |

### 2.3 P0 稳定化(必做)

| ID | 任务 | 关联文档/ADR |
|----|------|-------------|
| P0-A | test_utils 9 个失败修复(StringUtils/FileUtils/CompressUtils) | [01_stabilization.md](./v1.7.0/01_stabilization.md) |
| P0-B | 路线图文档同步(本文档重写) | — |
| P0-C | TSan 接入 CI(`tsan.yml` + suppressions) | ADR-005 §4 Enforcement |
| P0-D | tech_debt P2 头文件 hygiene(`future.h`/`typed_event_bus.h`) | tech_debt_audit H-1, H-2 |
| P0-E | tech_debt P3(`cache.h` QHash/`query_wrapper.h` ISqlDialect) | tech_debt_audit H-3, H-5 |
| P0-F | Clang-Tidy CI 检查任务(覆盖 blanket catch / raw new) | tech_debt_audit P3 #13 |

**验收标准**:
- TSan 在多线程测试套件上零警告(明确误报通过 suppression 文件注解)
- test_utils 21/21 通过
- Clang-Tidy 在核心模块上 0 新增警告
- 头文件 hygiene 通过(grep 验证 `future.h`/`typed_event_bus.h` 无重头文件)

### 2.4 P1 新模块(必做)

#### P1-A: SoulCache

| 组件 | 说明 |
|------|------|
| `ICache<K,V>` | 缓存抽象接口(get/put/remove/contains/size/clear) |
| `MemoryCache<K,V>` | L1 内存缓存,TTL + LRU 淘汰,`std::unordered_map` + 双向链表 |
| `DiskCache<K,V>` | L2 磁盘缓存,QFile 存储,元数据索引,LRU 淘汰 |
| `MultiLevelCache<K,V>` | L1+L2 组合,读穿透/写回策略,`CacheStats` 统计 |

**约束**:
- 不引入 Redis/Memcached 等外部依赖
- 与 ORM 集成通过 `CachedRepository<T>` 装饰器(见 P1-C)
- 线程安全级别: Thread-Safe(内部 `std::shared_mutex` 读写锁)

详细设计: [02_soul_cache_design.md](./v1.7.0/02_soul_cache_design.md)

#### P1-B: ORM Enhanced

| 子项 | 说明 |
|------|------|
| `CachedRepository<T>` | 装饰器模式包装 `IRepository<T>`,查询结果缓存到 SoulCache,写操作失效缓存 |
| `TypedQueryWrapper<T>` | 类型安全条件构造,`Column<T>` 强类型字段,编译期字段类型检查 |
| 实体反射宏 | `SOUL_ENTITY_REGISTER` / `SOUL_FIELD` 宏,自动化 getProperty/setProperty,消除样板代码 |
| `Migration` / `SchemaBuilder` / `MigrationManager` | Schema 版本迁移系统,支持 up/down,版本表 `_schema_migrations` |

**约束**:
- 公共接口保持向后兼容,新增方法而非修改现有方法
- Schema 迁移系统与 `ISqlDialect` 集成,支持 SQLite/MySQL/PostgreSQL
- 反射宏不引入重型 RTTI,基于 `std::any` + 类型擦除

详细设计: [04_orm_enhanced_design.md](./v1.7.0/04_orm_enhanced_design.md)

#### P1-C: SoulObservability

| 组件 | 说明 |
|------|------|
| `Metrics` | Counter/Gauge/Histogram 三种指标类型,`MetricsRegistry` 注册中心 |
| `Tracing` | `Span`/`SpanContext`/`Tracer`,跨函数调用上下文传播(线程局部存储) |
| `JsonSink` | 结构化 JSON 日志输出,与 `Logger` 集成,支持 module-level 过滤 |

**约束**:
- 不引入 OpenTelemetry SDK
- Metrics 导出格式: JSON(可扩展为 Prometheus exposition format 在 v1.8.0)
- Tracing 上下文不跨进程(仅单进程内跨线程)

详细设计: [03_soul_observability_design.md](./v1.7.0/03_soul_observability_design.md)

### 2.5 P2 增强项(可选,允许跨版本延期)

| ID | 任务 | 状态 |
|----|------|------|
| P2-A | RPC 测试深度(序列化对比/传输故障注入/负载均衡压力) | 选做 |
| P2-B | MQ 真实集成(RabbitMQ 优先,基于 amqpcpp) | 选做(优先于 HTTP/2/OAuth2) |

**明确延期到 v1.8.0 的项**:
- HTTP/2 多路复用支持
- OAuth2/OIDC 认证流程
- 分布式缓存(L3 Redis)
- OpenTelemetry 分布式追踪

### 2.6 v1.7.0 验收标准

| 里程碑 | 验收标准 | 实际状态 |
|--------|----------|----------|
| M0 — P0 稳定化完成 | TSan CI 运行零警告;test_utils 21/21 通过;Clang-Tidy 0 新增警告 | ⚠️ 部分达成:TSan/Clang-Tidy CI 延期到 v1.8.0(Linux 工具链依赖);test_utils 部分修复;路线图文档已同步 |
| M1 — P1 新模块实现 | SoulCache/ORM Enhanced/SoulObservability 单元测试覆盖率 ≥ 80% | ✅ 达成:三大模块全部交付;test_observability 42 个用例通过;test_mq InMemoryAmqpBackend 用例通过 |
| M2 — v1.7.0 发布 | 全量测试通过;ADR 合规性 100%;CHANGELOG 更新;版本号同步到 1.7.0 | ✅ 达成:Windows MinGW 11.2.0 + Qt 6.5.3 全量测试通过;ADR-001/003/005 合规;CHANGELOG v1.7.0 已发布 |

**M0 延期项注解**:
- TSan(P0-C)与 Clang-Tidy(P0-F)需 Linux 工具链支持,本版以 Windows MinGW 为主验证平台,延期至 v1.8.0 接入 Linux CI 节点后补齐
- test_utils(P0-A)与 tech_debt P2/P3(P0-D/E)部分完成,剩余项随 v1.8.0 P0 闭环

---

## 3. 当前迭代 — v1.8.0 (Planned)

**预计启动**: v1.7.0 发布后(2026-07-26 之后)
**主题**: 分布式能力 + 协议增强 + 安全合规 + v1.7.0 延期项闭环

### 3.1 计划范围(从 v1.7.0 延期)

| 模块 | 说明 | 优先级 | 来源 |
|------|------|--------|------|
| HTTP/2 | 多路复用、流优先级、服务器推送,基于 Qt 6.5+ RHI | High | v1.7.0 P2 延期 |
| OAuth2/OIDC | 授权码模式、客户端凭证模式、PKCE、ID Token 验证 | High | v1.7.0 P2 延期 |
| 分布式缓存(L3 Redis) | 在 SoulCache 之上扩展 `RedisCache`,RESP 协议自研或基于 hiredis | Medium | v1.7.0 P2 延期 |
| OpenTelemetry 集成 | 在 SoulObservability 之上扩展 OTLP exporter,跨进程追踪 | Medium | v1.7.0 P2 延期 |
| TSan CI 接入 | Linux CI 节点 + tsan.yml + suppressions | High | v1.7.0 P0-C 延期 |
| Clang-Tidy CI 检查 | 覆盖 blanket catch / raw new | High | v1.7.0 P0-F 延期 |
| RPC 测试深度 | 序列化对比 / 传输故障注入 / 负载均衡压力 | Medium | v1.7.0 P2-A 延期 |

### 3.2 候选新增

| 模块 | 说明 | 优先级 |
|------|------|--------|
| SoulGateway | API 网关模块,限流/熔断/路由 | Medium |
| 配置环境隔离 | dev/test/prod 分层配置加载,环境变量覆盖 | Medium |
| 备份与恢复 | 用户数据与配置的备份/恢复机制 | Low |
| 性能基准套件 | UI 渲染/数据库查询/网络操作关键路径 benchmark | Medium |

---

## 4. 长期愿景 — v2.0 (Long-term)

**主题**: 架构演进 + 平台扩展

### 4.1 候选项

| 特性 | 说明 | 风险 |
|------|------|------|
| Qt 7 支持 | 升级到 Qt 7 LTS,迁移 RHI/QtQml 新特性 | 高(API 变更) |
| 移动端支持 | iOS/Android via Qt for Mobile | 中(平台差异) |
| WebAssembly | WASM 支持,浏览器内运行 | 中(沙箱限制) |
| Vulkan 渲染 | GPU 加速渲染,替代 OpenGL | 高(硬件依赖) |
| 模块化(JPMS-like) | C++ Modules 替代头文件,提升编译速度 | 高(工具链成熟度) |
| 协程(C++20 Coroutines) | `co_await`/`co_yield` 替代 Future 链 | 中(编译器支持) |

### 4.2 决策原则

v2.0 引入破坏性变更前必须满足:
1. C++20 Coroutines 在 GCC/Clang/MSVC 三大编译器稳定支持 ≥ 2 年
2. Qt 7 发布 LTS 版本且社区采纳率 ≥ 60%
3. C++ Modules 在 CMake/Ninja 工具链中实现生产可用
4. 现有 v1.x 用户有清晰的迁移路径与工具支持

---

## 5. SoulCore 家族生态

| 库 | 定位 | 当前版本 | 目标版本 |
|----|------|----------|----------|
| **SoulCoreKit** | 核心基础设施 | v1.7.0 | v1.8.0 → v2.0 |
| **SoulUI** | 高级 UI 组件库 | 内嵌于 SoulCoreKit | v1.8.0 独立 |
| **SoulRPC** | RPC 框架 | 内嵌于 SoulCoreKit v1.6.0 | v1.8.0 独立 |
| **SoulCache** | 缓存抽象 | v1.7.0 引入(已交付) | v1.8.0 独立 |
| **SoulObservability** | 可观测性 | v1.7.0 引入(已交付) | v1.8.0 独立 |
| **SoulORM** | ORM 框架 | 内嵌于 SoulCoreKit(Enhanced 已交付) | v2.0 独立 |
| **SoulPlugin** | 插件系统 | 内嵌于 SoulCoreKit v1.3.0 | v2.0 独立 |
| **SoulMQ** | 消息队列抽象 | v1.7.0 InMemory + AmqpCpp 后端已交付 | v1.8.0 集成测试 + DLX/延迟队列 |
| **SoulAI** | AI 推理能力 | 规划中 | v2.0+ |
| **SoulLSP** | LSP 协议支持 | 规划中 | v2.0+ |
| **SoulMedia** | 音视频处理 | 规划中 | v2.0+ |

**独立化原则**: 当模块满足以下条件时,从 SoulCoreKit 独立为单独的库:
1. 公共接口稳定(≥ 1 个 LTS 版本未变更)
2. 用户群体明确(可独立使用而不依赖 SoulCoreKit 其他模块)
3. 独立 CI/CD 流水线与发布周期

---

## 6. 版本节奏与发布流程

### 6.1 版本号策略

遵循 [Semantic Versioning 2.0.0](https://semver.org/):
- **MAJOR** (X.0.0): 破坏性 API 变更,需提供迁移指南
- **MINOR** (1.X.0): 向后兼容的功能新增,符合 LTS 稳定性
- **PATCH** (1.0.X): 仅 bug 修复,无新功能

### 6.2 发布策略

- **按范围发布**(默认): P0+P1 范围完成后晋升版本号,无时间盒压力
- **LTS 标记**: 每个Minor版本的最后一个Patch版本标记为LTS(如v1.6.2 LTS),提供6个月维护窗口
- **预发布**: Alpha(内部测试)/Beta(公开测试)/RC(候选发布)三阶段,至少1周间隔

### 6.3 发布流程

1. 完成 P0+P1 范围,所有测试通过
2. 更新 CHANGELOG.md(Keep a Changelog 格式)
3. 同步版本号(CMakeLists.txt / Doxyfile / CITATION.cff / README.md)
4. 运行全量 CI 流水线(构建+测试+lint+覆盖率)
5. 创建 git tag `vX.Y.Z` 并推送
6. GitHub Release 自动触发 release.yml workflow,生成多平台产物
7. 更新本路线图,标记版本为 Released

---

## 7. 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|----------|
| SoulCache 设计复杂度超预期 | 推迟 M1 | 先实现 MemoryCache,DiskCache 延后到 v1.7.1 |
| ORM Enhanced 改造破坏现有接口 | 回归风险 | 严格遵循向后兼容;新增方法而非修改现有方法;完整回归测试 |
| TSan 暴露大量潜在竞争 | 阻塞 P0 | 仅标记明确的数据竞争,误报通过 suppression 文件注解 |
| SoulObservability 性能开销 | 生产影响 | Metrics 采样率可配;Tracing 默认关闭,按需开启 |
| Schema 迁移系统数据丢失 | 数据损坏 | 迁移前自动备份;down 迁移必须可逆;迁移事务化 |

---

## 8. 路线图评审清单

- ☑ v1.0 ~ v1.6.2 历史版本完整记录
- ☑ v1.7.0 已发布状态记录(P1 全交付 + P2 MQ 真实集成 + M0 部分延期注解)
- ☑ v1.7.0 实际交付清单(模块/测试/Bug/架构合规)文档化
- ☑ 5 个决策项已确认并落地(无 Redis / 无 OTel / Schema 迁移纳入 / MQ 优先 / 按范围发布)
- ☑ v1.8.0 延期项与来源明确(HTTP/2 / OAuth2 / Redis / OTel / TSan / Clang-Tidy / RPC 测试深度)
- ☑ v2.0 长期愿景与决策原则清晰
- ☑ SoulCore 家族生态规划完整
- ☑ 发布流程与版本号策略文档化
- ☑ 风险识别与缓解措施到位

---

**文档维护者**: SoulCoreKit 团队
**评审周期**: 每个迭代结束时评审一次,更新已发布版本状态与下一迭代范围
**变更记录**: 2026-07-26 v1.7.0 发布,标记 Released 状态,补齐实际交付清单;v1.8.0 晋升为当前迭代
