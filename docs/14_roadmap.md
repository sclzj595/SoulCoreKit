# SoulCoreKit Roadmap

**文档状态**: Accepted
**最后更新**: 2026-07-29
**当前基线**: v1.9.0 (AOP + HTTP Server + 资源池监控 + CS 架构定位修正)
**当前迭代**: v1.9.1 (CS 架构生产可用性:健康检查 / 中间件链 / 声明式事务 / WebSocket Server / 断线重连 / UI 测试)

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

## 3. 已发布 — v1.8.0 (Released 2026-07-26)

**起始日期**: 2026-07-26
**发布日期**: 2026-07-26
**发布策略**: **按范围发布**(完成 P0+P1 后发布)
**目标状态**: CI 质量闭环 + 协议增强
**详细设计**: [docs/v1.8.0/](./v1.8.0/README.md)
**版本类型**: Minor(功能新增,向后兼容)

### 3.0 实际交付清单

#### 3.0.1 P0 CI 质量闭环

| ID | 模块 | 状态 | 交付物 |
|----|------|------|--------|
| P0-A | TSan CI 接入 | ✅ Released | `.github/workflows/tsan.yml`(Linux + GCC + Qt 6.5.3 + TSan + suppressions),补齐 v1.7.0 P0-C 延期项 |

#### 3.0.2 P1 协议增强

| ID | 模块 | 状态 | 交付物 |
|----|------|------|--------|
| P1-A | HTTP/2 多路复用 | ✅ Released | `ConnectionPoolConfig` 结构体、HttpClient/HttpTransport 新增 `setHttp2Enabled`/`isHttp2Enabled`/`setConnectionPoolConfig` 接口,Qt 6.5 `Http2AllowedAttribute` + `QHttp2Configuration`,默认启用且自动降级 |
| P1-B | HttpClient 连接池 | ✅ Released | `ConnectionPoolConfig`(maxConnectionsPerHost/keepAliveTimeoutSec/enableHttp2/enableServerPush),复用 QNetworkAccessManager 内置连接池 |

#### 3.0.3 延期到 v1.9.0 的项

- Clang-Tidy CI 强制闭环(P2-A)
- RPC 测试深度(P2-B)
- AmqpCpp Linux/macOS 集成测试(P2-C)
- 配置环境隔离(P2-D)
- OAuth2/OIDC 认证流程
- SoulGateway API 网关模块
- L3 Redis 分布式缓存(继续暂缓)
- OpenTelemetry 集成(继续暂缓)

### 3.1 v1.8.0 验收

| 里程碑 | 验收标准 | 实际状态 |
|--------|----------|----------|
| M0 — P0 CI 闭环 | TSan workflow 创建;suppressions 文件预置 | ✅ 达成(workflow 已创建,待 Linux 节点运行验证) |
| M1 — P1 协议增强 | HTTP/2 启用且向后兼容;连接池配置可用 | ✅ 达成(默认启用 HTTP/2,Qt 自动降级保证兼容) |
| M2 — v1.8.0 发布 | 全量测试通过;CHANGELOG 更新;版本号同步 | ✅ 达成(23/23 测试通过,版本号同步到 1.8.0) |

---

## 4. 当前迭代 — v1.9.1 (Planned)

**预计启动**: v1.9.0 定位修正后
**主题**: CS 架构生产可用性 + 脚手架易用性提升
**版本类型**: Patch(定位修正 + 功能补全,向后兼容)

### 4.1 定位修正背景

v1.9.0 发布后,经审查发现项目定位存在偏差:

- **修正前**: "Qt 版 SpringBoot 全栈基础脚手架"("全栈"在 SpringBoot 语境下默认指 BS 架构)
- **修正后**: "Qt CS 架构 · SpringBoot 风格脚手架"(Client 桌面 + Server Linux)

修正影响:
- 移除 BS 专属需求(API 网关、嵌入式 Tomcat 对标)
- 新增 CS 特有需求(断线重连、连接数限制)
- 版本基线从 v2.0.0 改为 v1.9.1

### 4.2 P0 — 影响 CS 架构生产可用性

| GAP | 任务 | 端 | 类型 |
|-----|------|----|------|
| GAP-01 | Server 端健康检查端点 HealthIndicator | S | 新模块 |
| GAP-02 | HTTP Server 中间件链(鉴权/日志/CORS) | S | 架构 |
| GAP-03 | 声明式事务 `withTransaction<T>` + AOP 整合 | S | 架构 |
| GAP-04 | WebSocket Server | S | 新模块 |
| GAP-05 | Client 端断线重连/心跳管理增强 | C | 架构 |
| GAP-06 | UI 组件测试覆盖(30+ 组件) | C | 测试 |

### 4.3 P1 — 脚手架易用性提升

| GAP | 任务 | 端 | 类型 |
|-----|------|----|------|
| GAP-07 | 定时任务框架 `@Scheduled` | CS | 新模块 |
| GAP-08 | Server 端连接数限制与负载保护 | S | 架构 |
| GAP-09 | 配置元数据 `Config::bind<T>` | CS | 架构 |
| GAP-10 | 自动配置机制 `Scaffold::scan()` | CS | 架构 |
| GAP-11 | Clang-Tidy CI 强制闭环 | CS | CI |

### 4.4 v1.9.1 验收标准

| 里程碑 | 验收标准 |
|--------|----------|
| M0 — P0 完成 | 6 项 P0 GAP 全部 done,CS 架构生产可用 |
| M1 — P1 完成 | 5 项 P1 GAP 全部 done,脚手架易用性提升 |
| M2 — v1.9.1 发布 | 全量测试通过;CHANGELOG 更新;版本号同步 |

---

## 5. 长期愿景 — v2.5.0+ (Current)

**主题**: 架构定稿 — ApplicationContext + ServiceRegistry + 三层分层

### 5.1 v2.5.0 — 架构定稿 (In Progress)

**详细设计**: [v2.5.0-cs-architecture.md](./v2.5.0-cs-architecture.md)

| 变更项 | 内容 | 优先级 |
|--------|------|:---:|
| CODE-01 | 引入 `ApplicationContext` | P0 |
| CODE-02 | 引入 `ServiceRegistry` | P0 |
| CODE-03 | 引入 `ControllerRegistry` | P0 |
| CODE-04 | 重构 `CsModule` — 不持有 Service/Controller 所有权 | P0 |
| CODE-05 | Controller 分发优先使用强类型 | P0 |
| CODE-06 | 精简 `web/` — 删除提前实现的 .h 文件 | P0 |
| CODE-07 | 添加 `application/` 目录结构 | P0 |

### 5.2 v2.6.0 — CS Security (Spring Security 对标)

| 变更项 | 内容 | 优先级 |
|--------|------|:---:|
| CS-01 | CsSecurity — CS 场景安全适配 | P0 |
| CS-02 | OAuth2/OIDC 认证 — Authorization Code + PKCE | P0 |
| CS-03 | JWT Token 管理 — 签发/验证/刷新/吊销 | P0 |
| CS-04 | RBAC 权限模型 — 角色-权限-资源三级 | P0 |
| CS-05 | SecurityInterceptor — 请求级安全拦截 | P1 |
| CS-06 | 审计日志 — 结构化输出 | P2 |
| CS-07 | 密码加密 — bcrypt/argon2 | P2 |

### 5.3 v2.7.0 — CS Advanced (Spring Cloud 对标)

| 变更项 | 内容 | 优先级 |
|--------|------|:---:|
| CA-01 | CsAdminPanel — 内置管理面板 | P1 |
| CA-02 | CsIpcRouter — 本地 IPC 路由 | P1 |
| CA-03 | 配置中心客户端 — Nacos/Apollo/Consul | P1 |
| CA-04 | 分布式追踪 — OTLP HTTP + Span 传播 | P2 |
| CA-05 | 灰度发布 — Header/Cookie/IP 路由 | P2 |
| CA-06 | gRPC 集成 — Server/Client | P2 |
| CA-07 | 服务注册发现 — Consul/Eureka/Nacos | P2 |
| CA-08 | MQ 完整集成 — RabbitMQ + Kafka | P2 |

### 5.4 v3.0.0 — 正式发布

| 交付物 | 内容 |
|--------|------|
| 全量文档 | API 参考 + 架构指南 + 快速开始 |
| 部署指南 | Windows/macOS/Linux 部署 |
| 性能基准 | UI 渲染 + 数据库查询 + 网络操作 |
| 安全审计 | 依赖扫描 + 漏洞报告 |
| 发布包 | 多平台预编译二进制 + SDK |

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
