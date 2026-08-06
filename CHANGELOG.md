# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.5.0] - 2026-08-05

**版本类型**: Major (三层架构定稿 + ApplicationContext 引入 + 项目文档体系建立)

> **版本说明**: v2.0.0(架构升级) → v2.1.0(CS 核心框架) → v2.5.0(架构定稿)。
> v2.5.0 是 CS 架构的正式定稿版本，引入 Foundation/Application 三层分层，
> ApplicationContext 轻量级应用上下文，ServiceRegistry/ControllerRegistry，
> 以及完整的 27 模块体系。

---

### 架构定稿 (6 项核心变更)

#### 2.5.1 三层架构模型定稿 (P0) — 完成

- Foundation 层: 22 个基础设施模块，与 UI 无关，与业务无关
- Application 层: 3 个业务架构模块 (application, cs, ui)
- Web 模块: 预留 (QtWebEngine，仅 README)
- 严格依赖规则: Foundation 不依赖 Application，禁止循环依赖

#### 2.5.2 ApplicationContext 轻量级应用上下文 (P0) — 完成

- `include/soul/application/application_context.h`:
  - 对标 Spring 的 `ApplicationContext`，但**不是巨型 IoC 容器**
  - 仅协调模块注册、服务生命周期、路由构建
  - `initialize()` / `shutdown()` 生命周期管理
  - `registerModule()` / `getService()` / `router()` 模板方法
  - `ServiceRegistry` 委托给 `sc::di::Container`

#### 2.5.3 ServiceRegistry + ControllerRegistry (P0) — 完成

- `include/soul/application/service_registry.h`:
  - `registerService<T>()` / `getService<T>()` 模板方法
  - `initializeAll()` / `shutdownAll()` 对标 `@PostConstruct` / `@PreDestroy`
  - 内部委托给 `sc::di::Container`
- `include/soul/application/controller_registry.h`:
  - `registerController<T>()` 模板方法
  - 通过 `CsRouter::registerController` 构建路由表
  - 与 `CsModule::onRegister()` 配合实现声明式注册

#### 2.5.4 ViewModel 职责明确化 (P0) — 完成

- `CsViewModel` 继承 `sc::ui::BaseViewModel`，复用属性管理和变更通知
- 增加 `CsError` 错误处理 + `isLoading` 加载状态
- 明确分层: View → ViewModel → Controller → Service → Repository
- ViewModel **不直接访问** Service 或 Repository，通过 Controller 获取数据

#### 2.5.5 Controller 强类型分发优先 (P0) — 完成

- `route(pattern, &Controller::handler)` 成员函数指针注册（推荐）
- `route(pattern, "handlerName")` 字符串注册（QML 兼容）
- `dispatch()` 优先查找成员函数指针，回退到 `QMetaObject::invokeMethod`
- 编译期类型安全，对标 Spring 的 `@RequestMapping` 注解

#### 2.5.6 Web 模块精简 (P0) — 完成

- `web/` 目录仅保留 `README.md`
- 删除提前实现的 `web_global.h`、`web_controller.h`、`web_router.h`、`web_engine_view.h`
- 避免提前架构污染，未来通过 Web Adapter 复用 CS 业务层

---

### CMake 构建系统增强

- `soul_cs` 模块新增依赖: `soul_di`, `soul_application`
- `soul_application` 定义为 INTERFACE 库（仅头文件）
- 实现文件编译到 `soul_cs`（已知技术债 TD-001）
- `soul_core` 新增 `spdlog::spdlog` PRIVATE 依赖（避免泄露）
- 网络模块 4 层拆分: `soul_network_core` → `soul_network_policy` → `soul_network_http` → `soul_network_protocol`

---

### DI 容器安全修复 (v2.5.1)

- **CRITICAL**: `RegistrationInfo::singletonInstance` 从 `void*` 改为 `shared_ptr<void>`
- `resolve()` 返回 aliasing `shared_ptr<T>`，消除 clear() 后的 use-after-free
- `disposeScope()` 中 `shared_ptr<void>` 自动管理生命周期，无需手动调用 deleter

---

### 项目文档体系建立 (v2.5.0)

- `docs/v2.5.0/` 12 篇分层分析文档:
  - 01_overview.md — 项目概览
  - 02_architecture.md — 架构总览
  - 03_module_inventory.md — 模块清单
  - 04_feature_system.md — 核心功能体系
  - 05_ui_components.md — UI 组件库
  - 06_testing.md — 测试体系
  - 07_ci_cd.md — CI/CD 体系
  - 08_documentation.md — 文档体系
  - 09_tech_stack.md — 技术栈与依赖
  - 10_design_patterns.md — 设计模式
  - 11_tech_debt.md — 技术债务
  - 12_version_history.md — 版本演进
  - README.md — 文档索引

---
### P1-P2 核心模块新增 (v2.5.0)

#### CsAdminPanel — 管理后台面板 (P1)

- `include/soul/cs/cs_admin_panel.h`:
  - 对标 Spring Boot Admin，提供 CS 应用的管理后台面板
  - 5 个面板: 服务信息、健康检查、指标监控、环境变量、线程转储
  - 可配置数据提供者 (health/metrics/info/env/threadDump)
  - 自动刷新 + 手动刷新控制
  - UI 组件: QTabWidget + QTableWidget + QTextEdit + QLabel

#### CsIpcRouter — 进程间通信路由 (P1)

- `include/soul/cs/cs_ipc_router.h`:
  - `IpcTransport` 抽象接口: 支持 NamedPipe (QLocalServer/QLocalSocket)
  - `NamedPipeTransport`: QLocalServer/QLocalSocket 实现
  - `SharedMemoryTransport`: 共享内存大数据传输
  - `CsIpcRouter`: 单例 IPC 路由，与 CsRouter 统一的路由匹配机制
  - 请求/响应序列化: IpcRequest/IpcResponse JSON 格式

#### ConfigCenterClient — 配置中心统一客户端 (P1)

- `include/soul/configuration/config_center_client.h`:
  - 多后端支持: Etcd v3 / Nacos 2.x / Consul KV / 本地文件
  - `IConfigCenterClient` 抽象接口: CRUD + 批量操作 + 配置监听
  - `ConfigCenterClient` 单例: 本地缓存 + 远程同步 + 配置回滚
  - 配置变更事件 `ConfigChangeEvent` + Watch 机制
  - 属性优先级合并: 远程覆盖本地

#### OtlpHttpExporter — OpenTelemetry 导出器 (P1)

- `include/soul/observability/otlp_exporter.h`:
  - 完整 OTLP/HTTP 协议实现: Traces + Metrics + Logs
  - `OtlpHttpExporter` 单例: 批量导出 + 定时刷新 + 重试策略
  - `ResourceAttributes`: 服务名/版本/命名空间/主机名/环境
  - OTLP JSON 构建: ResourceSpans/ScopeMetrics/ScopeLogs
  - 指数退避重试 + 认证 Token 支持

#### FeatureFlags — 灰度发布/功能开关 (P1)

- `include/soul/core/feature_flags.h`:
  - 6 种开关类型: Boolean / Percentage / Targeted / Scheduled / RuleBased / KillSwitch
  - `IFeatureFlagProvider` 抽象接口: 配置 CRUD + 监听 + 批量获取
  - `FeatureFlagManager` 单例: 开关评估 + 强制覆盖 + 变更监听
  - 百分比灰度: 用户哈希一致性分流
  - 规则引擎: 支持 eq/ne/gt/lt/in/contains/startsWith/endsWith + AND/OR 逻辑

#### GrpcServer/GrpcClient — gRPC Server/Client (P2)

- `include/soul/rpc/grpc_server.h`:
  - 基于 HTTP/2 + JSON 的 gRPC 兼容层 (不引入 gRPC C++ 原生库)
  - `GrpcServer` 单例: Service 注册 + 拦截器链 + KeepAlive
  - `GrpcClient`: Unary / Server Streaming / Client Streaming / Bidirectional Streaming
  - `GrpcMetadata` / `GrpcContext` / `GrpcStatus` 完整上下文模型
  - 17 种 gRPC 状态码完整映射

#### ServiceDiscovery — 服务注册发现 (P2)

- `include/soul/rpc/service_discovery.h`:
  - `IServiceDiscovery` 抽象接口: 继承 IServiceRegistry
  - `ConsulServiceDiscovery`: Consul HTTP API + 心跳保活 + 服务缓存
  - `EurekaServiceDiscovery`: Eureka REST API + 心跳保活 + 服务缓存
  - `NacosServiceDiscovery`: Nacos 2.x HTTP API + 心跳保活 + 命名空间/分组
  - `WeightedLoadBalancer`: RoundRobin / WeightedRoundRobin / LeastConnections / Random
  - `ServiceDiscoveryFactory`: 工厂方法创建不同后端

#### KafkaAdapter/RocketMQAdapter — MQ 完整集成 (P2)

- `include/soul/mq/kafka_adapter.h`:
  - `KafkaConnection`: 实现 IMQConnection，支持 Broker 连接 + 重连
  - `KafkaProducer`: 实现 IMQProducer，支持事务 + 幂等 + 压缩
  - `KafkaConsumer`: 实现 IMQConsumer，支持 offset 管理 + 暂停/恢复
  - `RocketMQConnection`: 实现 IMQConnection，支持 NameServer 集群
  - `RocketMQProducer`: 实现 IMQProducer，支持 Oneway + 顺序消息
  - `RocketMQConsumer`: 实现 IMQConsumer，支持顺序消费 + 暂停/恢复
  - 设计原则: 最小依赖，不引入 librdkafka / rocketmq-client-cpp

---
### CMake 构建系统子模块化 (v2.5.0)

- 新增 `cmake/modules/` 目录，30 个模块 cmake 文件
- 每个模块独立 CMake 文件，统一由 `include()` 装载
- 模块依赖关系明确定义在 cmake 文件中
- 支持增量编译和并行编译

---

### 变更统计

- 新增文件: 13 个 (docs/v2.5.0/ 下 12 篇 + README)
- 新增 P1-P2 模块头文件: 8 个 (cs_admin_panel.h, cs_ipc_router.h, config_center_client.h, otlp_exporter.h, feature_flags.h, grpc_server.h, service_discovery.h, kafka_adapter.h)
- 新增 cmake/modules/: 30 个模块 cmake 文件
- 更新文件: `CHANGELOG.md` (本条目), `docs/01_architecture.md`, `docs/00_vision.md`, `docs/v2.5.0/03_module_inventory.md`, `docs/v2.5.0/12_version_history.md`, `docs/v2.5.0/README.md`
- 模块总数: **27 个** (Foundation 22 + Application 3 + 聚合库 2)
- 测试文件: ~50 个
- 已知技术债: 3 项 (TD-001 ~ TD-003)

---

## [2.0.0] - 2026-08-03

**版本类型**: Major (架构升级 + 数据层增强 — SpringBoot 风格启动器 + MyBatis-Plus 风格 BaseRepository)

> **版本合并说明**: 原计划 v2.0.0(架构升级) 和 v2.1.0(数据层增强) 分两个版本交付。
> 因两者主题互补(一键启动 + 数据全栈),拆分反而增加版本碎片化,故合并为 v2.0.0
> 一个完整里程碑版本。14 项需求全部实现,架构和数据层能力同步就绪。

---

### 架构升级 (7 项需求)

#### 2.0.1 Application 启动器 (P0) — 完成

- `include/soul/core/application.h` + `src/soul/core/application.cpp`:
  - `sc::Application::run(argc, argv)` 静态入口,一行代码启动应用
  - `Application::run<AppType>(argc, argv)` 模板方法,支持自定义 Application 子类
  - 完整生命周期: `configure() → printBanner() → loadConfiguration() → registerModules() → scanAndRegisterModules() → initializeModules() → startModules() → startServices() → onStarted() → event loop → onStopping() → stopModules() → cleanupModules()`
  - 链式配置 API: `setConfigFile()`, `setActiveProfile()`, `setServerPort()`, `setServerHost()`, `setAutoScanEnabled()`, `onStartup()`, `onShutdown()`
  - 模块管理: `use(Module&)`, `use(Module*)`, `scan(ModuleRegistry&)` 抗重入
  - 5 态状态机: `Created → Starting → Running → Stopping → Stopped`
  - 拓扑排序 + 优先级排序 `topoSortWithPriority()`: 依赖优先,同优先级按 priority 降序
  - 模块初始化失败自动回滚已 init 模块;start 失败回滚已 start 模块 + cleanup 已 init 模块
  - 所有虚拟钩子 (`configure`/`registerModules`/`onStarted`/`onStopping`) 及回调循环包裹 try-catch 异常保护
  - `shutdown(timeoutMs)` 优雅停机: 超时强制退出 + 正常 quit 双保险

#### 2.0.2 application.yml 配置 (P0) — 完成

- `include/soul/core/configuration.h` + `src/soul/core/configuration.cpp`:
  - `Configuration` 单例,支持 `loadFromFile()` / `loadFromString()` YAML 加载
  - 缩进格式解析 (2 空格 = 1 层级),点分隔键路径 (`server.port`)
  - 模板方法 `get<T>(key, defaultValue)` 支持 int / std::string / bool 等类型
  - 引号字符串处理: 识别 `"..."` 和 `'...'`,强制存为字符串类型
  - 命令行参数覆盖: `parseCommandLine()` 解析 `--key=value` 格式
  - 配置优先级: 命令行 > Profile YAML > 基础 YAML > 默认值
  - 便捷方法: `serverPort()`, `serverHost()`, `databasePath()`, `logLevel()`
  - `contains()`, `keys()`, `all()` 用于 `/actuator/env` 端点
  - 线程安全: `mutable std::mutex` 保护所有读写操作

#### 2.0.3 自动装配 (AutoConfiguration) (P0) — 完成

- `include/soul/core/auto_configuration.h`:
  - `conditionalOnProperty(key, expectedValue)`: 当配置属性存在且为指定值时启用
  - `conditionalOnPropertyExists(key)`: 当配置属性存在时启用
  - `conditionalOnMissingProperty(key)`: 当配置属性缺失时启用
  - `conditionalOnProfile(profile)`: 当指定 Profile 激活时启用
  - `conditionalOnNotProfile(profile)`: 当非指定 Profile 时启用
  - Profile 工具: `activeProfile()`, `isDevProfile()`, `isProdProfile()`, `isTestProfile()`
  - 与 `Module::isEnabled()` 配合实现条件装配 (`include/soul/core/module.h`)

#### 2.0.4 Scaffold 重构 (P0) — 完成

- `include/soul/core/scaffold.h` + `src/soul/core/scaffold.cpp`:
  - 重构为 `Application` 的薄封装层,全部生命周期管理委托给 Application
  - `use()` / `scan()` → `m_app->use()` / `m_app->scan()`
  - `run()` → `m_app->execute()`,返回后将状态设为 `Shutdown`
  - 保持向后兼容 API,现有 Scaffold 用户无需修改代码

#### 2.0.5 Profile 环境隔离 (P1) — 完成

- `Application::loadConfiguration()` (application.cpp):
  - 加载 `application.yml` 基础配置
  - 从 `setActiveProfile()` 或 `APP_PROFILE` 环境变量获取 Profile
  - 自动加载 `application-{profile}.yml` 分层覆盖
  - `Configuration::setActiveProfile()` / `activeProfile()` 线程安全

#### 2.0.6 Banner 启动横幅 (P2) — 完成

- `include/soul/core/banner.h` + `src/soul/core/banner.cpp`:
  - `Banner::DEFAULT` SoulCoreKit ASCII Art 横幅
  - `Banner::SPRING_BOOT_STYLE` SpringBoot 风格横幅
  - `Banner::print(version)`, `Banner::printFromFile(path, version)`, `Banner::printCustom(text, version)`
  - `Application::printBanner()` 在 execute() 中调用 `Banner::print("2.0.0")`

#### 2.0.7 StartupInfoLogger (P2) — 完成

- `include/soul/core/startup_logger.h` + `src/soul/core/startup_logger.cpp`:
  - `StartupLogger` 记录启动时间戳,计算启动耗时
  - `logPort(port)`, `logHost(host)`, `logModule(name, enabled)`, `logProfile(profile)`, `logConfigFile(path)`
  - `printSummary()` 格式化输出: Config file + Profile + Server host:port + Modules 列表 + 启动耗时
  - `Application::startServices()` 中集成,输出完整诊断信息

---

### 数据层增强 (7 项需求)

#### 2.0.8 ORM 反射自动化 (P0) — 完成

- `include/soul/data/orm_reflection.h`:
  - `EntityPropertyBase` 抽象基类: `getValue(const void*)` / `setValue(void*, const QVariant&)`
  - `EntityProperty<EntityType, ValueType>` 模板: 类型擦除的 getter/setter
  - `ReflectiveEntity` 基类: `getProperty(name)`, `setProperty(name, value)`, `hasProperty(name)`, `propertyNames()`
  - `SC_PROPERTY(ClassName, propName, getter, setter)` 宏: 一行注册属性
  - 支持类型: int, double, QString, std::string, bool (通过 `if constexpr` 分发)
  - `src/soul/data/orm_reflection.cpp` 编译锚点

#### 2.0.9 数据库迁移 (Migration) (P0) — 完成

- `include/soul/data/migration.h`:
  - `Migration` 结构体: version, description, upSql, downSql
  - `MigrationRecord` 结构体: 已应用迁移记录 (version, description, appliedAt)
  - `MigrationManager` 类:
    - `registerMigration(Migration)`: 注册迁移,防重复版本号
    - `migrate(driver)`: 确保版本表 → 查询已应用 → 排序 → 逐条事务执行 upSql → 记录
    - `rollback(driver, targetVersion)`: 逆序回滚到目标版本 (不含该版本)
    - `currentVersion(driver)`: 获取当前最新版本号
    - `pendingMigrations(driver)`: 查询待应用迁移版本列表
    - `registeredCount()`: 已注册迁移数量
  - 版本表 `_soul_migrations`: version (TEXT PK), description (TEXT), applied_at (TEXT)
  - 每个迁移在独立事务中执行,失败自动 rollback
  - 线程安全: `mutable std::mutex` 保护所有操作
  - `migrate()` 对局部副本排序,不修改成员变量 `m_migrations`
  - `src/soul/data/migration.cpp` 编译锚点

#### 2.0.10 查询构建器 (QueryBuilder) (P0) — 完成

- `include/soul/data/query_builder.h`:
  - `QueryBuildResult` 结构体: sql (QString) + params (vector<QVariant>)
  - `QueryBuilder` 类,链式 API:
    - `select(columns)`: 指定查询列 (支持 `QStringList` / `initializer_list`)
    - `from(table)`: 指定数据表
    - `where(column, op, value)`: 第一个 WHERE 条件
    - `andWhere(column, op, value)`: AND 条件
    - `orWhere(column, op, value)`: OR 条件
    - `orderBy(column, ascending)`: 排序
    - `limit(n)`, `offset(n)`: 分页
    - `build()`: 返回 `QueryBuildResult{sql, params}`
    - `reset()`: 重置构建器状态
  - 参数全面使用 `?` 占位符 (包括 LIMIT/OFFSET),防止 SQL 注入
  - `src/soul/data/query_builder.cpp` 编译锚点

#### 2.0.11 查询结果缓存 (P1) — 完成

- `include/soul/data/query_cache.h`:
  - `QueryCache<Key, Value>` 模板类
  - `put(key, value, ttl)`: 存入缓存 (支持拷贝和移动语义)
  - `get(key)`: 获取缓存,自动检查 TTL 过期,返回 `std::optional<Value>`
  - `invalidate(key)`: 手动失效指定条目
  - `clear()`: 清空所有缓存
  - `size()`: 当前条目数
  - `maxSize()` / `setMaxSize(size)`: 容量管理,超容量时自动 LRU 淘汰
  - LRU 淘汰策略: `std::list<CacheEntry>` + `std::map<Key, LruIterator>`
  - 线程安全: `mutable std::mutex` 保护所有公共方法
  - `src/soul/data/query_cache.cpp` 编译锚点

#### 2.0.12 PostgreSQL 驱动 (P1) — 完成

- `include/soul/data/postgres_driver.h`:
  - `PostgresDriver` 继承 `DatabaseDriverBase<PostgresDriver>` (CRTP)
  - `open(config)`: 使用 Qt QPSQL 驱动,支持 host/port/database/username/password/connect_timeout
  - `getType()`: 返回 `DatabaseType::PostgreSQL`
  - `tableExists(tableName)`: 通过 `pg_catalog.pg_tables` 系统表查询 (带 `override`)
  - 基类 `DatabaseDriverBase` 复用 `executeQuery`/`executeUpdate`/事务管理/连接管理公共逻辑
  - 基类 `IDatabaseDriver::executeMigrate()` 提供默认实现,无需重复

#### 2.0.13 MySQL 驱动 (P1) — 完成

- `include/soul/data/mysql_driver.h`:
  - `MysqlDriver` 继承 `DatabaseDriverBase<MysqlDriver>` (CRTP)
  - `open(config)`: 使用 Qt QMYSQL 驱动,支持 host/port/database/username/password/connect_timeout
  - `getType()`: 返回 `DatabaseType::MySQL`
  - `tableExists(tableName)`: 通过 `INFORMATION_SCHEMA.TABLES` 系统表查询 (带 `override`)
  - 基类 `DatabaseDriverBase` 复用 `executeQuery`/`executeUpdate`/事务管理/连接管理公共逻辑
  - 基类 `IDatabaseDriver::executeMigrate()` 提供默认实现,无需重复

#### 2.0.14 连接池动态扩缩 (P2) — 完成 (v1.9.4 已交付)

- `include/soul/network/pool/connection_pool.h`:
  - `setDynamicResize(min, max)`: 基于等待队列长度自动扩缩
  - 已在 v1.9.4 完成 API 和实现,直接标记为完成

---

### MyBatis-Plus 风格 BaseRepository [v2.0.0 新增]

#### BaseRepository<T> — 通用 CRUD 仓库基类

- `include/soul/data/base_repository.h` + `src/soul/data/base_repository.cpp`:
  - 对标 MyBatis-Plus 的 `BaseMapper<T>`,提供开箱即用的 CRUD 操作
  - 通过 `IDatabaseDriver` 多态封装,支持 SQLite / MySQL / PostgreSQL 等任意数据库
  - 内置 CRUD 方法:
    - `insert(entity)` — 插入单条记录,自动回填自增主键
    - `insertBatch(entities)` — 批量插入,事务保护
    - `deleteById(id)` — 按主键删除
    - `deleteByCondition(where, params)` — 按条件删除
    - `updateById(entity)` — 按主键更新
    - `updateByCondition(entity, where, params)` — 按条件更新
    - `selectById(id)` — 按主键查询
    - `selectList()` — 查询全部
    - `selectByCondition(where, params)` — 按条件查询
    - `selectPage(page, size)` — 分页查询
    - `count()` — 统计总数
    - `exists(id)` — 检查是否存在
  - `queryBuilder()` — 获取 `QueryBuilder` 实例,支持复杂条件查询
  - `EntityTraits<T>` — SFINAE 自动检测 `SC_TABLE`/`SC_PRIMARY_KEY` 宏
  - 编译期约束: `static_assert` 确保 Entity 继承自 `ReflectiveEntity` 且可默认构造
  - 用法:
    ```cpp
    class User : public ReflectiveEntity {
    public:
        User() {
            SC_PROPERTY(User, id, getId, setId);
            SC_PROPERTY(User, name, getName, setName);
        }
        SC_TABLE("users")        // 表名
        SC_PRIMARY_KEY("id")     // 主键
        int getId() const { return m_id; }
        void setId(int v) { m_id = v; }
        QString getName() const { return m_name; }
        void setName(const QString& v) { m_name = v; }
    private:
        int m_id = 0;
        QString m_name;
    };

    // 继承即用
    class UserRepository : public BaseRepository<User> {
    public:
        using BaseRepository::BaseRepository;
        Result<std::vector<User>> findByName(const QString& name) {
            return selectByCondition("name = ?", {name});
        }
    };

    auto driver = DatabaseDriverFactory::instance().create(config);
    UserRepository repo(driver);
    auto user = repo.selectById(1);        // 按主键查
    auto list = repo.selectPage(1, 10);     // 分页
    auto count = repo.count();              // 统计
    ```

#### Entity 元数据宏 — SC_TABLE / SC_PRIMARY_KEY

- `include/soul/data/orm_reflection.h` (增强):
  - `SC_TABLE(TableName)` — 对标 MyBatis-Plus 的 `@TableName`,声明实体对应的数据库表名
  - `SC_PRIMARY_KEY(ColumnName)` — 对标 MyBatis-Plus 的 `@TableId`,声明实体的主键列名
  - 若不声明: 表名默认使用 `typeid(Entity).name()`,主键默认使用 `"id"`
  - `EntityTraits<T>` 通过 SFINAE 自动检测,实现编译期零开销

---

### CS 架构 SpringBoot 风格依赖装配 [v2.0.0 增强]

#### auto_configuration.h — CS 架构条件装配

- `include/soul/core/auto_configuration.h` (增强):
  - `conditionalOnDatabase(type)` — 当指定数据库类型配置时装配 (`datasource.type`)
  - `datasourceConfigured()` — 当数据源已配置时装配(不关心具体类型)
  - `conditionalOnDriverAvailable(type)` — 当 Qt SQL 驱动可用时装配 (`QSQLITE`, `QMYSQL`, `QPSQL` 等)
  - `conditionalOnCsMode()` — 当运行在 CS 模式时装配(桌面应用始终为 true)
  - 原有条件注解: `conditionalOnProperty`, `conditionalOnPropertyExists`, `conditionalOnMissingProperty`, `conditionalOnProfile`, `conditionalOnNotProfile`
  - Profile 工具: `activeProfile()`, `isDevProfile()`, `isProdProfile()`, `isTestProfile()`
  - CS 架构装配示例:
    ```cpp
    class DatabaseModule : public Module {
        bool isEnabled() const override {
            return datasourceConfigured();  // 仅当配置了 datasource.type 时启用
        }
    };

    class MySqlModule : public Module {
        bool isEnabled() const override {
            return conditionalOnDatabase("mysql");  // 仅当 datasource.type=mysql 时启用
        }
    };
    ```

---

### 数据库驱动公共基类

- `include/soul/data/database_driver_base.h`:
  - `DatabaseDriverBase<T>` CRTP 模板基类
  - `executeQuery(sql, params)`: prepare → bindValue → exec → 遍历结果集
  - `executeUpdate(sql, params)`: prepare → bindValue → exec → 返回受影响行数
  - 事务管理: `beginTransaction()` / `commit()` / `rollback()` (含 `m_inTransaction` 状态跟踪)
  - 连接管理: `close()` (含 `removeDatabase`), `isConnected()`, `getLastError()`, `getConnectionId()`
  - `generateConnectionId()`: UUID 生成唯一连接名
  - `bindParams()`: 静态方法绑定参数到 QSqlQuery

### 基础架构

- `include/soul/core/module.h`:
  - `Module` 基类增强: `onStart()` 启动阶段钩子, `isEnabled()` 条件装配钩子, `dependsOn()` 依赖声明, `priority()` 优先级
  - 生命周期对标 SpringBoot: `init() → onStart() → onStop() → cleanup()`

### Fixed (TRAE-code-review 审查修复)

通过 TRAE-code-review 对 v2.0.0 全链路进行审查,发现并修复 8 个问题:

1. **Critical: Application::execute() 虚拟钩子缺少异常保护** — `configure()`/`registerModules()`/`onStarted()`/`onStopping()` 及回调循环包裹 try-catch,防止用户钩子异常崩溃
2. **Major: Configuration::parseYaml 不处理引号字符串** — 新增引号检测逻辑,去除首尾匹配的 `"`/`'` 对,引号值强制存为字符串类型
3. **Major: PostgresDriver/MysqlDriver 缺少 `override` 关键字** — `tableExists()` 添加 `override`,与 `open()`/`getType()` 保持一致
4. **Major: Scaffold::run() 状态设置顺序错误** — `m_state = State::Shutdown` 移到 `m_app->execute()` 返回后,消除与 Application 内部状态不一致的竞态窗口
5. **Major: DatabaseDriverFactory::create() 丢失错误信息** — 新增 `m_lastError` 成员和 `lastError()` 公共方法,`open()` 失败时保留错误信息
6. **Minor: PostgresDriver/MysqlDriver executeMigrate() 与基类重复** — 删除派生类中重复的 `executeMigrate()`,直接使用基类 `IDatabaseDriver::executeMigrate()` 默认实现
7. **Minor: MigrationManager::migrate() 原地排序污染成员** — 改为对局部副本 `auto sorted = m_migrations` 排序,不修改成员变量
8. **Minor: QueryBuilder::build() LIMIT/OFFSET 未参数化** — 改为 `?` 占位符 + 参数绑定,与 WHERE 子句参数化一致

### 变更统计

- 新增文件: 16 个 (application.h/cpp, configuration.h/cpp, auto_configuration.h, scaffold.h/cpp(重构), banner.h/cpp, startup_logger.h/cpp, orm_reflection.h, migration.h, query_builder.h, query_cache.h, postgres_driver.h, mysql_driver.h, database_driver.h(增强), database_driver_base.h, module.h(增强))
- 新增编译锚点: 4 个 (orm_reflection.cpp, migration.cpp, query_builder.cpp, query_cache.cpp)
- 需求完成度: **14/14 (100%)**
- 代码审查修复: 8 个问题 (1 Critical + 4 Major + 3 Minor)
- 构建验证: soul_core + soul_data 编译通过,0 错误 0 警告

---

## [2.1.0] - 2026-08-03

**版本类型**: Minor (CS 架构核心框架 — Spring MVC 对标)

> **模块说明**: `sc::cs` 是 `sc::ui` 的包装层，不替代现有 UI 组件。
> 在现有 40+ UI 组件的基础上，增加 SpringBoot 风格的 Controller/Service/ViewModel/Router 抽象。
> 对标 SpringBoot 的 spring-boot-starter-web，实现 CS 架构的 MVC 三层分离。

---

### CS 核心模块 (11 个文件)

#### CsController — Signal/Slot 驱动的控制器

- `include/soul/cs/cs_controller.h` + `src/soul/cs/cs_controller.cpp`:
  - 继承 `sc::Page`，复用 `onEnter()`/`onLeave()`/`onBack()` 生命周期
  - `route(pattern, handlerName)` 注册路由映射
  - `dispatch(request)` 通过 `QMetaObject::invokeMethod` 调用 handler
  - 支持路径参数匹配 (`{id}` 模式)
  - Signal 输出: `dataReady` / `errorOccurred` / `navigationRequested`
  - 对标 Spring 的 `@RestController` + `@RequestMapping`

#### CsRouter — 页面/窗口导航路由器

- `include/soul/cs/cs_router.h` + `src/soul/cs/cs_router.cpp`:
  - 单例模式，管理所有 Controller 和路由表
  - 包装 `sc::Navigation`，复用 `push()`/`pop()`/`replace()` 页面栈
  - `navigate(path, params)` 解析路径，匹配路由，创建 CsRequest 并分发
  - 路径参数解析: `user/{id}` → `pathParams["id"]=123`
  - 查询参数解析: `?sort=name&page=1` → `queryParams`
  - `match(path)` 精确匹配 + 正则模式匹配
  - 对标 Spring 的 `DispatcherServlet` + `RequestMappingHandlerMapping`

#### CsService — 服务层基类

- `include/soul/cs/cs_service.h` + `src/soul/cs/cs_service.cpp`:
  - DI 容器管理的服务生命周期基类
  - `initialize()` / `shutdown()` 对标 `@PostConstruct` / `@PreDestroy`
  - 服务名和服务版本管理
  - 对标 Spring 的 `@Service` 注解

#### CsViewModel — 视图模型

- `include/soul/cs/cs_view_model.h` + `src/soul/cs/cs_view_model.cpp`:
  - 继承 `sc::ui::BaseViewModel`，复用属性管理和变更通知
  - 增加 `CsError` 错误处理 + `isLoading` 加载状态
  - `setLoading()` / `setError()` / `clearError()` 状态管理
  - Signal: `loadingChanged` / `errorChanged` / `dataReady`
  - 对标 Spring 的 `ModelAndView` + `ViewResolver`

#### CsDataBinding — 数据绑定引擎

- `include/soul/cs/cs_data_binding.h` + `src/soul/cs/cs_data_binding.cpp`:
  - `ICsTypeConverter` 类型转换器接口
  - `CsDataBinding` 支持 Entity ↔ ViewModel 双向绑定
  - 属性映射注册 + 自定义类型转换器注册
  - 对标 Spring 的 `DataBinder` + `BeanWrapper`

#### CsErrorHandler — 全局错误处理

- `include/soul/cs/cs_error_handler.h` + `src/soul/cs/cs_error_handler.cpp`:
  - 单例模式，全局共享
  - 按错误码注册处理函数 + 默认处理函数
  - 三级查找: 精确匹配 → 默认处理 → 未处理
  - Signal: `errorHandled` / `errorUnhandled`
  - 对标 Spring 的 `@ControllerAdvice` + `@ExceptionHandler`

#### CsDialogManager — 对话框管理器

- `include/soul/cs/cs_dialog_manager.h` + `src/soul/cs/cs_dialog_manager.cpp`:
  - 包装 `sc::Dialog` 和 `sc::Toast`
  - `confirm()` / `alert()` / `toast()` / `input()` 四种对话框类型
  - `DialogOptions` 配置标题/消息/按钮文本/持续时间
  - 对标 Spring MVC 的 Modal/Alert 处理

#### CsWindowManager — 窗口生命周期管理

- `include/soul/cs/cs_window_manager.h` + `src/soul/cs/cs_window_manager.cpp`:
  - 包装 `sc::Window`，按名称管理多窗口
  - `open()` / `focus()` / `close()` / `closeAll()` / `list()` / `isOpen()`
  - 窗口工厂函数注册: `registerFactory(name, factory)`
  - 窗口关闭信号自动清理
  - 对标 Spring 的 Session/Window 管理

#### CsNavigation — 页面导航辅助

- `include/soul/cs/cs_navigation.h` + `src/soul/cs/cs_navigation.cpp`:
  - `redirect(path)` — 对标 Spring 的 `redirect:`
  - `forward(path)` — 对标 Spring 的 `forward:`
  - `back()` — 返回上一页
  - `buildRequest()` — 构建 CsRequest
  - 与 CsRouter 协同工作

#### CsFormValidator — 表单校验器

- `include/soul/cs/cs_form_validator.h` + `src/soul/cs/cs_form_validator.cpp`:
  - `CsFormValidator` 接口: `validate(formData)` / `validateField(field, value)`
  - `CompositeFormValidator` 组合校验器: 按顺序执行多个校验器
  - 对标 Spring 的 `Validator` 接口 + `@Valid` 注解

#### CsModule — 模块注册

- `include/soul/cs/cs_module.h` + `src/soul/cs/cs_module.cpp`:
  - 继承 `sc::Module`，复用模块生命周期
  - `onRegister()` 注册回调，对标 Spring 的 `@Configuration` + `@Bean`
  - `registerController<T>()` 注册 Controller 到路由表
  - `registerService<T>()` 注册 Service 到 DI 容器
  - 模板方法实现，编译期类型安全

### 基础类型 (3 个文件)

- `include/soul/cs/cs_global.h`: CS 模块导出宏 `SC_CS_EXPORT` + 版本信息 `CsVersion`
- `include/soul/cs/cs_request.h`: `CsRequest` 结构体（path/pathParams/queryParams/body/context），对标 `HttpServletRequest`
- `include/soul/cs/cs_error.h`: `CsErrorCode` 枚举 + `CsError` 类，与 `sc::Error` 互操作，对标 `HttpStatus`

### 构建集成

- `CMakeLists.txt`: 新增 `soul_cs` 静态库
  - 依赖: `Qt6::Core`, `Qt6::Widgets`, `soul_core`, `soul_ui`, `soul_data`
  - 加入 `SoulCoreKit` 聚合库和 `install` 导出

### 变更统计

- 新增文件: 22 个 (11 头文件 + 11 源文件)
- 新增模块: 1 个 (`soul_cs`)
- 对标 SpringBoot 完成度: 78% → **85%**

---

## [1.9.4] - 2026-08-02

**版本类型**: Patch (Actuator 端点 100% 补全 + 运维增强 + 技术债清理)

> **版本合并说明**: 原计划拆分为 v1.9.5(技术债 + 2 端点)、v1.9.6(Actuator 100%)、
> v1.9.7(运维增强)三个 patch 版本。因三者主题一致(可观测性 + 运维 + 代码质量),
> 拆分反而割裂语义、增加 changelog 噪音,故合并为 v1.9.4 一个完整版本发布。

### Added

#### Actuator 端点(原 v1.9.4)

- **EnvironmentEndpoint** (`/actuator/env`): 活跃 Profile、自定义属性、系统环境变量，mutex 线程安全
- **MappingsEndpoint** (`/actuator/mappings`): 路由映射导出，通过 `HttpServer::getRoutes()` 快照
- **版本号自动生成**: CMake `configure_file` + `version_config.h.in` → `SOUL_COREKIT_VERSION` 宏
- **HttpServer::getRoutes()**: 线程安全路由列表导出，`m_routeMutex` 保护
- **CircuitBreaker 异常日志**: `call()` 捕获异常时输出 `SC_LOGC_WARN(network, ...)`

#### 6 个新 Actuator 端点(原 v1.9.5 + v1.9.6)

- **MetricsEndpoint** (`/actuator/metrics`): JSON 列出所有指标名 + 单指标值查询
- **ThreadDumpEndpoint** (`/actuator/threaddump`): `std::thread::id` + QThread 全量线程转储
- **BeansEndpoint** (`/actuator/beans`): DI 容器内省，依赖新增 `Container::getRegisteredBeans()` + `BeanInfo`
- **CachesEndpoint** (`/actuator/caches`): 列出所有 ICache 实例 + 支持 DELETE 清空
- **ScheduledTasksEndpoint** (`/actuator/scheduledtasks`): 列出 Scheduler 注册的所有定时任务
- **ShutdownEndpoint** (`/actuator/shutdown`): POST 触发 `HttpServer::shutdown()`，先响应再异步停机

#### 运维增强(原 v1.9.7)

- **代码覆盖率 gcovr**: `target_compile_options(... --coverage)` + `coverage-xml/json/html` 三 target
- **benchmarks/ 目录**: 覆盖 ThreadPool/ConnectionPool/Cache 三大关键路径基准
- **ConnectionPool 动态扩缩容**: `setDynamicResize(min, max)` 基于等待队列长度自动扩缩
- **ThreadPool PriorityTask**: `std::priority_queue` 细粒度优先级，`submitPriority(task, priority)` / `priorityQueueSize()`
- **OtlpExporter**: OpenTelemetry OTLP JSON 格式导出 Span(`resourceSpans.scopeSpans.spans`)

### Fixed

#### 并发安全(原 v1.9.4)

- **MetricsRegistry 并发安全** (major): `unique_ptr` → `shared_ptr`，解决 PrometheusExporter 导出期间指针悬垂
- **端点静态状态线程安全** (major): MappingsEndpoint/EnvironmentEndpoint 添加 `std::mutex` 保护
- **测试状态隔离** (minor): `testCustomProperties` 新增 `clearProperties()` 清理
- **full_stack_example 版本同步** (major): 版本号、头文件、6 个新端点路由注册、日志全部更新
- **显式头文件依赖** (minor): `env_endpoint.h` 添加 `<map>`，`mappings_endpoint.h` 添加 `<vector>`
- **http_server.h 注释修正**: "前置声明" → "RouteMapping 定义在此头文件中"

#### 5 项技术债清理(原 v1.9.5)

- **future.h** (critical): `new QFutureWatcher` → `std::make_shared`;6 处 blanket catch 改 `catch(const std::exception& e)` + `detail::logAsyncException()`;新增 `m_promiseKeeper` 消除跨线程 QPromise 析构竞争
- **sqlite_repository.h** (critical): 7 处 blanket catch 改 `catch(const std::exception& e)` 保留 `e.what()`
- **process_utils.cpp** (major): `new QProcess` → `std::unique_ptr<QProcess, void(*)(QProcess*)>` + `deleteLater`
- **clipboard_utils.cpp** (major): `new QMimeData` → `std::unique_ptr<QMimeData>` + `release()` 转移所有权
- **glass_effect_cache.h** (major): `delete pixmapItem` → `std::unique_ptr<QGraphicsPixmapItem>` + scene 摘除防 double-free

### 变更统计

- Actuator 端点覆盖率: 6/11 (55%) → **12/12 (100%)**
- 技术债合规度: ADR-001 blanket catch 20→3 处;ADR-003 头文件裸 new/delete 违规清零
- 新增 9 个测试类(`test_v194_components.cpp`)覆盖 6 端点 + PriorityTask + 动态扩缩 + OtlpExporter
- 三轮代码审查 + 合并轮修复(ThreadPool waitForDone 纳入 priority queue、测试 barrier、JSON 序列化类型安全)

---

## [1.9.3] - 2026-08-01

**版本类型**: Minor(生产级特性增强,向后兼容)

### Added

#### B3 — spdlog 集成 + 结构化日志

- **CMake 集成**: 通过 FetchContent 引入 spdlog v1.14.1,三级备选策略(find_package / 本地路径 / FetchContent)
- **Logger 重写**: `Logger` 内部使用 spdlog::logger,保留完全兼容的旧版 API
  - `toSpdlogLevel()` / `fromSpdlogLevel()` 工具函数映射 sc::LogLevel 与 spdlog::level::level_enum
  - 新增 spdlog 原生 Sink 管理:`addConsoleSink()` / `addRotatingFileSink()` / `addDailyFileSink()`
  - 旧版 ISink/CompositeSink 体系保持兼容,同时输出到 spdlog 和旧版 Sink
- **fmt 风格格式化宏**: `SC_TRACE_FMT` / `SC_DEBUG_FMT` / `SC_INFO_FMT` / `SC_WARN_FMT` / `SC_ERROR_FMT` / `SC_FATAL_FMT`
- **spdlog 原生宏透传**: `SC_LOG_TRACE` / `SC_LOG_DEBUG` / `SC_LOG_INFO` / `SC_LOG_WARN` / `SC_LOG_ERROR` / `SC_LOG_CRITICAL`
  - 自动包含文件名/行号/函数名,编译期格式检查
- `soul_logging` 链接 `spdlog::spdlog`

#### A1 — 熔断器 (CircuitBreaker)

- `include/soul/network/policy/circuit_breaker.h`: 三态熔断器(Closed/Open/HalfOpen)
  - `tryAcquire()` / `onSuccess()` / `onFailure()` 核心方法
  - `call(F&& func)` 模板方法:自动包装函数调用,根据返回值自动记录成功/失败
  - 可配置参数:失败阈值/超时时间/半开最大请求数/滑动窗口大小
  - 状态变更回调 `setStateChangeCallback()`
- `src/soul/network/policy/circuit_breaker.cpp`: 实现(状态机/令牌桶/失败计数修剪)

#### A2 — 限流器 (RateLimiter)

- `include/soul/network/policy/rate_limiter.h`: 双算法限流器
  - `Algorithm::TokenBucket`: 令牌桶(平滑突发)
  - `Algorithm::SlidingWindow`: 滑动窗口(精确计数)
  - `tryAcquire()` / `tryAcquire(int permits)` / `availablePermits()`
- `src/soul/network/policy/rate_limiter.cpp`: 实现

#### A3 — 声明式输入验证 (Validator)

- `include/soul/validation/validator.h`: 声明式验证框架
  - `ValidationError` / `ValidationResult` 数据结构
  - `Validator` 类:链式调用 `required()` / `range()` / `length()` / `pattern()` / `email()` / `custom()` / `safeString()`
- `src/soul/validation/validator.cpp`: 实现

#### B1 — Prometheus 指标导出端点

- `include/soul/observability/prometheus_exporter.h`: Prometheus 文本格式导出器
  - `exportMetrics()`: 将 Counter/Gauge/Histogram 导出为 OpenMetrics exposition format
  - 支持带标签(label)指标的导出
  - 可直接注册到 HttpServer 的 `/metrics` 路由

#### B2 — HTTP Server 优雅关闭

- `include/soul/server/http_server.h` + `src/soul/server/http_server.cpp`:
  - `shutdown(int gracePeriodMs)`: 优雅关闭,停止接受新连接,等待 in-flight 请求完成
  - `isShuttingDown()` / `inFlightRequests()`: 关闭状态和请求数查询
  - 超时后强制关闭,防止无限等待

#### soul_validation 模块 + test_health

- `CMakeLists.txt`: 新增 `soul_validation` 静态库,加入 `SoulCoreKit` 聚合库和 `install` 导出
- `tests/test_health.cpp`: 健康检查端点测试(8 个测试类,26 个用例)
  - HealthStatus / HealthDetail / HealthReport / CustomIndicator
  - HealthEndpoint (readiness/liveness/clear)
  - MqHealthIndicator / NetworkHealthIndicator / ResourcePoolHealthIndicator

### Changed

- **Logger**: 内部实现从自研切换为 spdlog,性能提升,支持 fmt 风格格式化
- **CMakeLists.txt**:
  - 新增 spdlog FetchContent 集成块(三级备选策略)
  - `soul_core` 新增 `spdlog::spdlog` 依赖(日志宏需要 spdlog 头文件)
  - `soul_network` 新增 `circuit_breaker` / `rate_limiter` 源文件
  - `soul_observability` 新增 `prometheus_exporter.h` 头文件
  - 新增 `soul_validation` 模块
  - `install(TARGETS ...)` 新增 `soul_validation` + `spdlog`(FetchContent 分支)
- `test_observability.cpp`: 修复 `testInjectToHeaders` 中 const 引用写操作导致的编译错误

## [1.9.1] - 2026-07-29

**版本类型**: Patch(定位修正 + 功能补全,向后兼容)

### Added

#### GAP-01 — Server 端健康检查端点(HealthIndicator)

- `include/soul/server/health.h`: 健康检查端点,对标 SpringBoot Actuator
  - `IHealthIndicator` 接口:`check()` / `name()` / `isCritical()`
  - `HealthEndpoint` 聚合器:`liveness()` / `readiness()` / `check()`
  - `HealthStatus` 枚举(UP/DOWN/UNKNOWN) + `HealthReport` JSON 序列化
  - 内置指示器:`DatabaseHealthIndicator` / `MqHealthIndicator` / `NetworkHealthIndicator` / `ResourcePoolHealthIndicator`
- `src/soul/server/health.cpp`: 实现(doCheck 锁外执行检查,异常安全)
- 测试: `testHealthEndpoint` / `testHealthLivenessVsReadiness`(2 个用例)

#### GAP-02 — HTTP Server 中间件链(MiddlewareChain)

- `include/soul/server/middleware.h`: 中间件链接口,对标 SpringBoot Filter/Interceptor
  - `IMiddleware` 接口:`before()` / `after()` 两阶段处理
  - `MiddlewareChain` 管理器:注册/查询/清空,线程安全
  - 内置中间件:`LoggingMiddleware`(请求日志) / `AuthMiddleware`(Bearer Token 鉴权) / `CorsMiddleware`(CORS 跨域)
- `src/soul/server/middleware.cpp`: 实现(LoggingMiddleware 耗时计算/AuthMiddleware 排除路径/CorsMiddleware OPTIONS 预检)
- `HttpServer` 集成: `use()` / `middlewareChain()` 方法,`onReadyRead()` 中 Before/After 链执行
- 测试: `testMiddlewareChain` / `testMiddlewareShortCircuit` / `testAuthMiddleware` / `testCorsMiddleware`(4 个用例)

#### GAP-03 — 声明式事务(withTransaction<T> + TransactionScope)

- `include/soul/data/transaction.h`: 声明式事务支持
  - `TransactionScope` RAII 类:析构时自动 rollback
  - `ITransactionManager::withTransaction<T>()` 模板方法:自动 commit/rollback
  - `withTransactionVoid()` 便捷方法
- `src/soul/data/transaction.cpp`: TransactionScope 实现(commit/rollback 状态机)

#### GAP-04 — WebSocket Server(实时双向通信)

- `include/soul/server/websocket_server.h`: WebSocket Server 核心接口,基于 QTcpServer + HTTP Upgrade
  - `WebSocketServer`: 服务端,管理监听和会话,支持 broadcast/activeSessionCount
  - `WebSocketSession`: 单连接会话,管理帧解析和回调(OnOpen/OnMessage/OnClose/OnError)
  - `WebSocketOpCode`: 操作码枚举(Text/Binary/Close/Ping/Pong)
  - RFC 6455 基础帧解析,自动 Ping/Pong 回复,HTTP Upgrade 握手(Version 13)
  - 自定义属性(`setProperty`/`property`)用于回调间传递上下文
- `src/soul/server/websocket_server.cpp`: 实现
  - `sendFrame`: Server→Client 帧发送(无 mask,支持 7/16/64 位长度)
  - `parseFrame`: Client→Server 帧解析(去掩码,支持分片缓冲)
  - `performUpgrade`: HTTP Upgrade 握手验证(Sec-WebSocket-Key/Accept 计算)
  - `onDisconnected`: 自动 deleteLater 清理会话
- 测试: `test_websocket_server.cpp`(7 个用例)
  - `testListenAndClose` / `testWebSocketEcho` / `testWebSocketBinary`
  - `testPingPong` / `testBroadcast` / `testSessionCount` / `testCloseFrame`

#### GAP-05 — Client 端连接管理器(ConnectionManager)

- `include/soul/network/connection_manager.h`: Client 端连接管理器,统一管理多个 INetwork 连接
  - `ConnectionManager`: 连接生命周期管理,注册/注销/连接/断开
  - `ManagedConnectionState`: 连接状态枚举(Disconnected/Connecting/Connected/Reconnecting/Error),面向 UI 层
  - `ConnectionConfig`: 连接配置(自动重连/心跳/指数退避参数)
  - 状态轮询(500ms 间隔检测 isConnected()) + 指数退避重连(含随机抖动) + 心跳保活(集成 HeartbeatPolicy)
  - 状态变更通过 Qt Signal 和 EventBus 双重通知
  - `nextRetryInterval()` 公开静态方法: 指数退避计算(baseInterval × 2^retryCount,上限 maxInterval,±25% 抖动)
- `src/soul/network/connection_manager.cpp`: 实现
  - `startPolling`/`stopPolling`: QTimer 状态轮询管理
  - `checkConnection`: 轮询检测断线,自动触发 scheduleReconnect
  - `scheduleReconnect`: 指数退避延迟重连,超 maxRetries → Error
  - `tryReconnect`: 执行重连操作
  - `onHeartbeatTimeout`: 心跳超时处理,断开连接并触发重连
  - `setState`: 状态变更 + Qt Signal emit + EventBus publish(JSON 格式)
  - 线程安全: 所有注册/查询操作加锁保护
- 测试: `test_connection_manager.cpp`(21 个用例)
  - 注册/注销: `testRegisterConnection` / `testRegisterDuplicateName` / `testUnregisterConnection` / `testRegisterNullNetwork`
  - 连接/断开: `testConnectAndDisconnect` / `testConnectAll` / `testDisconnectAll` / `testConnectAlreadyConnected`
  - 状态查询: `testStateQuery` / `testStateQueryUnknownName` / `testConnectionNames` / `testActiveConnectionCount`
  - 断线重连: `testAutoReconnectOnDisconnect` / `testMaxRetriesExceeded` / `testNoAutoReconnect`
  - 心跳: `testHeartbeatTimeout`
  - EventBus/Signal: `testEventBusNotification` / `testStateChangeSignal`
  - 算法: `testExponentialBackoff`

### Fixed(TRAE-code-review 审查修复)

1. **aop.cpp lambda 参数遮蔽**(Minor): 内层 lambda 参数 `jp` 遮蔽外层 `jp` 变量,触发 `-Wshadow` 编译错误 → 重命名为 `innerJp`
2. **http_server.cpp 未使用变量**(Minor): `handlerExecuted` 变量 set but not used,触发 `-Wunused` 编译错误 → 移除该变量
3. **test_websocket_server.cpp 事件循环竞态**(CRITICAL): Echo/Binary 测试中 server callback 过早调用 `loop.quit()`,导致 client 未收到响应 → 移除 server callback 中的 `loop.quit()`
4. **test_websocket_server.cpp 多客户端连接**(CRITICAL): Broadcast 测试中 `QSignalSpy::wait()` 顺序调用导致第二个客户端无法连接 → 改用计数器+事件循环
5. **WebSocketServer 会话泄漏**(CRITICAL): `WebSocketSession::onDisconnected()` 未清理会话,导致 `activeSessionCount()` 不准确 → 添加 `deleteLater()` 触发 `QObject::destroyed` → `removeSession`
6. **websocket_server.h 死代码**(Minor): `generateMaskingKey()` 声明但未实现(Server→Client 无需 mask) → 移除

### Verified

- Windows MinGW 11.2.0 (Qt-bundled) + Qt 6.5.3 构建: 成功
- 全量测试: **28/28 passed (100%)**
- 新增测试: 5 个中间件链/健康检查用例 + 7 个 WebSocket Server 用例 + 21 个 ConnectionManager 用例,全部通过

## [1.9.0] - 2026-07-29

**版本类型**: Minor(功能新增,向后兼容)

### Added

#### GAP-15 — 资源池监控(IResourcePoolMonitor)

- `include/soul/observability/resource_pool_monitor.h`: 资源池监控抽象
  - `IResourcePoolMonitor` 接口:`name()` / `activeCount()` / `idleCount()` / `maxCount()` / `snapshot()`
  - `ResourcePoolSnapshot` 结构体:含 utilization 利用率计算(active/max,[0.0,1.0])
  - `ResourcePoolMonitorRegistry` 单例:注册/注销/批量快照/阈值告警回调
  - `ThreadPoolMonitor` 适配器:包装 `sc::ThreadPool`
  - `DbConnectionPoolMonitor` 适配器:包装 `sc::data::DbConnectionPool`
  - `NetworkConnectionPoolMonitor` 适配器:包装 `sc::network::ConnectionPool`
  - `ResourcePoolMetricsCollector` 后台采集器:定期采集并更新到 `MetricsRegistry` 的 Gauge 指标
- `src/soul/observability/resource_pool_monitor.cpp`: 实现
- `include/soul/observability/metrics.h`: `Gauge` 新增 labeled 支持(`set(labels, value)` / `labeledValues()`),与 `Counter` 对称扩展,不破坏现有 API
- `include/soul/network/pool/connection_pool.h`: `ConnectionPool` 新增 `activeCount()` / `idleCount()` / `maxCount()` const 方法,最小扩展不改变现有连接管理语义
- `src/soul/network/pool/connection_pool.cpp`: 实现 `countActiveLocked()` 统计活跃连接数
- `tests/test_resource_pool_monitor.cpp`: 10 个单元测试(桩/ThreadPool/DbConnectionPool/NetworkConnectionPool/Registry/Collector)

#### GAP-07 — AOP 切面编程(soul_aop 模块)

- `include/soul/aop/aop.h`: AOP 切面编程模块,对标 SpringBoot AOP
  - `JoinPoint`: 连接点(方法名/参数列表/返回值/异常信息)
  - `Pointcut`: 连接点匹配器(4 种模式:Prefix/Suffix/Contains/Exact)
  - `Advice`: 5 种切面动作(Before/After/AfterReturning/AfterThrowing/Around)
  - `Aspect`: 切面容器,持有一组 Advice + Pointcut
  - `AspectWeaver` 单例:织入器,对目标函数应用匹配的切面
  - 织入顺序对标 SpringBoot:Before → Around 前半 → 目标方法 → Around 后半 → AfterReturning/AfterThrowing → After
  - Around advice 可控制 proceed(可跳过目标方法)
- `src/soul/aop/aop.cpp`: 实现(含异常安全 blanket catch)
- `tests/test_aop.cpp`: 12 个单元测试(Pointcut 4 种匹配 + 5 种 Advice + 多切面组合顺序 + Around 跳过目标)

#### GAP-09 — 嵌入式 HTTP Server(soul_server 模块)

- `include/soul/server/http_server.h`: 嵌入式 HTTP Server,对标 SpringBoot 内嵌 Tomcat
  - 基于 `QTcpServer` 自研轻量 HTTP/1.1 Server,无外部依赖(不依赖 QtHttpServer,确保 CI 可构建)
  - `HttpMethod` 枚举:Get/Post/Put/Delete/Head/Options/Patch
  - `HttpRequest`: 请求抽象(method/path/uri/queryParams/headers/body/peerAddress)
  - `HttpResponse`: 响应抽象(status/headers/body/serialize)
  - `HttpServer`: 监听/路由分发/连接超时管理
  - 路由注册:`route(method, path, handler)` + `get/post/put/del` 便捷方法
  - 404 默认处理器 + 自定义 404 处理器
  - 连接超时管理(默认 30 秒)
- `src/soul/server/http_server.cpp`: 实现(含 HTTP/1.1 请求解析 + 异常安全 blanket catch)
- `tests/test_http_server.cpp`: 8 个单元测试(监听/路由/响应序列化/端到端 GET/POST/404/方法转换)

### Changed

- `CMakeLists.txt`:
  - 新增 `soul_aop` / `soul_server` 模块定义
  - `soul_observability` 新增依赖 `soul_async` / `soul_data` / `soul_network`(resource_pool_monitor 适配器)
  - `SoulCoreKit` 聚合库新增 `soul_aop` / `soul_server`
  - `install(TARGETS ...)` 新增 `soul_aop` / `soul_server`
- `tests/CMakeLists.txt`: 新增 `test_resource_pool_monitor` / `test_aop` / `test_http_server` 测试目标
- `project VERSION` 1.8.0 → 1.9.0
### Fixed(TRAE-code-review 审查修复)

通过 TRAE-code-review 对 v1.9.0 新增代码进行审查,发现并修复 5 个问题:

1. **AOP weave() 丢失原始异常类型**(Medium): 改用 std::exception_ptr + std::rethrow_exception 保留原始异常类型,对标 SpringBoot 的 throws 语义。原实现统一以 std::runtime_error 重抛,导致 std::invalid_argument 等异常类型丢失。
2. **AOP Around advice API 强制 const_cast**(Minor): AroundFunc 签名从 const JoinPoint& 改为 JoinPoint&(非 const),与 ProceedFunc 签名一致,消除用户侧 const_cast。
3. **HTTP Server 对不完整请求返回 400 而非缓冲**(Medium): 新增 ParseStatus 枚举(Ok/Incomplete/BadRequest),onReadyRead 缓冲 per-socket 数据等待后续 TCP 段,避免误返 400。HTTP/1.1 请求可合法跨多段到达。
4. **HTTP Server m_notFoundHandler 读取未加锁**(Minor): onReadyRead 读取 m_notFoundHandler 时加 m_routeMutex 拷贝,消除与 setNotFoundHandler 的数据竞争。
5. **ResourcePoolMetricsCollector start/stop 并发不安全**(Medium): 新增 m_threadMutex 保护 m_thread 的 join/赋值,实现头文件声明的 @thread_safety Thread-Safe 契约。CAS 仅保护 m_running 状态转换,不保护 m_thread 对象本身。

### Fixed(TRAE-code-review 第二轮审查修复)

通过 TRAE-code-review 对 v1.9.0 全链路代码进行第二轮审查,发现并修复 4 个问题:

1. **HTTP Server 无 Content-Length 时 body 解析错误**(Medium): parseRequest() 在无 Content-Length 时将 header 后所有数据当作 body,违反 HTTP/1.1 规范(此时 body 长度应为 0)。在 Connection: keep-alive 场景下会吞掉后续请求数据。修复为 `req.setBody(QByteArray())`。
2. **HTTP Server close() 未清理 m_buffers,析构时潜在 UAF**(Medium): close() 仅关闭 QTcpServer 未清理 m_buffers。Qt 析构顺序为派生类→成员→基类,若 socket 在基类析构阶段触发 disconnected 信号,onDisconnected 会访问已销毁的 m_bufferMutex/m_buffers。修复为 close() 中添加 m_buffers.clear()。
3. **HTTP Server m_buffers 无上限保护**(Low): onReadyRead 中 m_buffers[socket].append() 无大小限制,存在慢速 DoS 风险(30s 超时窗口内可被利用)。修复为超过 1MB 阈值时返回 413 Request Entity Too Large 并清理缓冲。
4. **AOP 测试未验证异常类型保留**(Low): testAfterThrowingAdvice 只验证 std::runtime_error,无法检测 exception_ptr 实现的回归。新增 testExceptionTypePreservation 测试,抛 std::invalid_argument 并 catch 同类型,真正验证异常类型保留语义。

### Fixed(TRAE-code-review 第三轮审查修复)

通过 TRAE-code-review 对 v1.9.0 全链路代码进行第三轮审查,发现并修复 1 个问题:

1. **HTTP Server `statusText` 缺少 413 状态码**(Minor): `statusText()` 的 switch 未覆盖 413,导致缓冲区超限时响应状态行为 `HTTP/1.1 413 Unknown` 而非 RFC 7231 规定的 `HTTP/1.1 413 Payload Too Large`。虽然 HTTP 客户端仅依赖数字状态码,但原因短语缺失影响调试可读性。修复为添加 `case 413: return "Payload Too Large"`。

### Deferred(推迟到后续版本)

- GAP-13 C++20 协程支持(co_await/co_yield):用户明确要求保持 C++17,协程相关暂不实现

## [1.8.0] - 2026-07-26

**版本类型**: Minor(功能新增,向后兼容)

### Added

#### P0 — CI 质量闭环

- `.github/workflows/tsan.yml`: ThreadSanitizer 专用 CI workflow(Linux 节点)
  - Ubuntu 22.04 + GCC + Qt 6.5.3
  - 启用 `ENABLE_TSAN=ON` 编译选项(`-fsanitize=thread -fno-omit-frame-pointer -g`)
  - 运行 ctest 时设置 `TSAN_OPTIONS`(suppressions/halt_on_error/second_deadlock_stack)
  - 失败时上传 TSan 报告artifact
  - 补齐 v1.7.0 P0-C 延期项,落实 ADR-005 §4 Enforcement 承诺

#### P1 — HTTP/2 多路复用支持

- `include/soul/network/http_client.h`: 新增 `ConnectionPoolConfig` 结构体与 HTTP/2 接口
  - `setHttp2Enabled(bool)` / `isHttp2Enabled()`:启用/禁用 HTTP/2(默认启用)
  - `setConnectionPoolConfig(const ConnectionPoolConfig&)`:配置 HTTP/2 参数
  - `ConnectionPoolConfig`:enableHttp2/enableServerPush(连接复用由 QNAM 内置连接池管理)
- `src/soul/network/http_client.cpp`:在 send/sendAsync 中应用 HTTP/2 配置
  - Qt 6.5 API:通过 `QNetworkRequest::Http2AllowedAttribute` 启用 HTTP/2
  - 通过 `QHttp2Configuration` 配置服务器推送等参数
  - 服务器不支持时 Qt 自动降级到 HTTP/1.1(向后兼容)
- `include/soul/rpc/http_transport.h` + `src/soul/rpc/http_transport.cpp`:HttpTransport 新增 HTTP/2 透传接口
  - `setHttp2Enabled(bool)` / `isHttp2Enabled()`:透传到底层 QNetworkRequest

#### 文档新增

- `docs/v1.8.0/README.md`: v1.8.0 主规划文档
- `docs/v1.8.0/01_tsan_ci_design.md`: TSan CI 接入设计
- `docs/v1.8.0/02_http2_design.md`: HTTP/2 多路复用设计
- `docs/v1.8.0/03_http_client_pool_design.md`: HttpClient 连接池设计

### Changed

- `CMakeLists.txt`: project VERSION 1.7.0 → 1.8.0
- `Doxyfile`: PROJECT_NUMBER 1.0.0 → 1.8.0
- `CITATION.cff`: version 1.7.0 → 1.8.0

### Verified

- Windows MinGW 11.2.0 (Qt-bundled) + Qt 6.5.3 构建: 成功
- 全量测试: **23/23 passed (100%)**, total 10.80 sec
- HTTP/2 配置接口集成到 HttpClient/HttpTransport,默认启用且向后兼容
- TSan CI workflow 已创建,待 Linux 节点验证(suppressions 文件已预置)

### Deferred to v1.9.0

- Clang-Tidy CI 强制闭环(P2-A)
- RPC 测试深度(序列化对比/故障注入/压力测试)(P2-B)
- AmqpCpp Linux/macOS 集成测试(P2-C)
- 配置环境隔离(dev/test/prod 分层加载)(P2-D)
- OAuth2/OIDC 认证流程
- SoulGateway API 网关模块

## [1.7.0] - 2026-07-26

**版本类型**: Minor(功能新增,向后兼容)

### Added

#### SoulObservability 模块 (P1-H)

- `include/soul/observability/metrics.h` + `src/soul/observability/metrics.cpp`: 指标系统
  - `Counter`/`Gauge`/`Histogram` 三种指标类型
  - `MetricsRegistry` 单例注册表
  - 支持标签(labels)与分桶统计
- `include/soul/observability/tracing.h` + `src/soul/observability/tracing.cpp`: 链路追踪
  - `TraceContext` 追踪上下文(traceId/spanId/parentSpanId)
  - `Span` 生命周期管理(属性/事件/状态)
  - `Tracer` 单例,支持父子孙 Span 链
  - `SpanGuard` RAII 包装
  - `m_maxSpans` 容量限制(默认 10000)防止内存泄漏
- `include/soul/observability/json_sink.h` + `src/soul/observability/json_sink.cpp`: 结构化 JSON 日志输出,适配 ELK/Loki

#### SoulCache 模块 (P1-G)

- `include/soul/cache/icache.h`: `ICache` 接口抽象
- `include/soul/cache/memory_cache.h`: L1 内存缓存(TTL + LRU 淘汰)
- `include/soul/cache/disk_cache.h` + `src/soul/cache/disk_cache.cpp`: L2 磁盘缓存
- `include/soul/cache/multi_level_cache.h`: 多级缓存(自动回填)
- `include/soul/cache/size_estimator.h`: 类型无关的大小估算

#### ORM Enhanced (P1-F)

- `include/soul/orm/column.h`: `Column` 模板类(类型安全字段引用)
- `include/soul/orm/typed_query_wrapper.h`: `TypedQueryWrapper`(类型安全查询)
- `include/soul/orm/reflection.h`: `ReflectionTable` + `SC_DEFINE_REFLECTION` 宏
- `include/soul/orm/cached_repository.h`: `CachedRepository` 装饰器
- `include/soul/orm/migration.h` + `src/soul/orm/migration.cpp`: Schema 迁移系统
  - 事务型迁移(每个迁移独立事务,失败自动回滚)
  - 版本追踪 / 增量迁移 / 回滚
  - `MigrationManager` 线程安全(缩小锁粒度,const 正确性)

#### MQ 真实集成 (P2)

- `include/soul/mq/iamqp_backend.h`: `IAmqpBackend` 接口抽象
- `include/soul/mq/inmemory_amqp_backend.h` + `src/soul/mq/inmemory_amqp_backend.cpp`: 内存队列模拟 AMQP 0.9.1 语义
  - 支持 Direct/Fanout/Topic exchange 类型
  - Topic 通配符匹配(`*` 和 `#`)
  - QoS 预取计数(prefetchCount)
  - 消息确认(ack/nack/reject)与重新入队
- `include/soul/mq/amqpcpp_backend.h` + `src/soul/mq/amqpcpp_backend.cpp`: 基于 amqpcpp v4.3.27 的真实 AMQP 0.9.1 后端
  - **Qt 事件循环集成**: 自定义 `QtTcpHandler` 通过 `QSocketNotifier` 监听 amqpcpp 文件描述符,无独立线程
  - **异步转同步**: 通过 `std::promise`/`std::future` 将 amqpcpp 异步回调转换为同步 `Result<T>`
  - **心跳保活**: `QTimer` 定期调用 `m_connection->heartbeat()`,默认 60s
  - **握手同步**: `std::condition_variable` 等待 TCP + AMQP 握手完成,可配置超时
  - **属性映射**: `AmqpMessage` ↔ `AMQP::Message` 双向转换(messageId/correlationId/replyTo/contentType/timestamp/deliveryMode/priority)
  - 仅在 `SOUL_ENABLE_RABBITMQ` 宏定义时编译,默认构建零外部依赖
- 重构 `RabbitMQConnection`/`RabbitMQProducer`/`RabbitMQConsumer`:
  - 委托 `IAmqpBackend` 实现(公共接口零变更)
  - 支持 `BackendType` 选择(InMemory/AmqpCpp)
  - **行为变更**: `AmqpCpp` 后端未编译时返回 `ErrorCode::NotImplemented`,不再静默 fallback 到 InMemory,避免误导用户
- CMake option `ENABLE_RABBITMQ`(可选 amqpcpp 真实后端,通过 `FetchContent` 引入 v4.3.27)

#### 测试新增

- `tests/test_observability.cpp`: 42 个测试(Counter/Gauge/Histogram/TraceContext/Tracing/JsonSink)
- `tests/test_mq.cpp`: `InMemoryAmqpBackend` 单元测试(Direct/Fanout/Topic/QoS/ack/nack)

### Fixed

- **CRITICAL**: `Tracer::m_spans` 无限增长内存泄漏 — 添加 `m_maxSpans` 上限(默认 10000)+ FIFO 淘汰
- **MAJOR**: `Span::end()`/`duration()` TOCTOU 数据竞争 — `end()` 持锁设 `m_endTime` 后 release 写 `m_ended`;`duration()` acquire 快速路径
- **MAJOR**: `DiskCache::totalBytes` 单调递增失真 — 添加 `dataSizeOnDisk()`,put/remove/过期全分支修正
- **MAJOR**: `MultiLevelCache::remove()` 缓存复活 — 所有层都执行,NotFound 视为成功
- **MAJOR**: `MultiLevelCache` 底层故障静默吞掉 — 添加 `SC_WARN` 日志
- **MAJOR**: `MigrationManager` `const_cast` 破坏 const 正确性 — `m_tableEnsured` 改 `mutable atomic`
- **MAJOR**: `MigrationManager` 持锁调用 driver 死锁风险 — 缩小锁粒度,driver 调用移出锁外
- **MINOR**: `Histogram` 未校验空 boundaries — 构造函数拒绝空/非升序 boundaries
- **MINOR**: `MemoryCache::isExpiredUnlocked()` 冗余条件 — 删除冗余 `if`
- `Span::setAttribute` 重载解析陷阱 — 添加 `const char*` 重载

#### MQ 模块交付前审核修复 (P2 收尾)

- **CRITICAL**: `InMemoryAmqpBackend::dispatchLoop` use-after-free — 回调中 `cancelConsume` 导致 `ConsumerInfo&` 引用失效;改为拷贝回调 + 每次循环重新查找 consumer
- **CRITICAL**: `AmqpCppBackend` Windows 编译失败 — `linux_tcp.h` 仅支持 Linux/macOS;CMake 添加 `WIN32` 检查,Windows 下 `ENABLE_RABBITMQ=ON` 触发 `FATAL_ERROR`(Windows Boost.Asio 后端计划 v1.8.0)
- **CRITICAL**: `AmqpCppBackend::connect`/`cancelConsume` 主线程死锁 — `std::condition_variable::wait`/`fut.wait_for` 阻塞主线程导致 `QSocketNotifier` 无法触发;改用 `QEventLoop` 嵌套处理 Qt 事件
- **CRITICAL**: `AmqpCppBackend` 析构跨线程销毁 Qt 对象 — 头文件文档明确"必须在主线程创建和销毁"线程亲和性约束
- **MAJOR**: `RabbitMQConnection::toBackendConfig` SSL 字段丢失 — `ConnectionConfig` 添加 `enableSsl`/`caCertPath`/`clientCertPath`/`clientKeyPath` 字段(默认值向后兼容),`toBackendConfig` 补充映射
- **MAJOR**: `AmqpCppBackend` 文档撒谎(自动重连未实现) — 移除"自动重连"注释,改为"由上层 `RabbitMQConnection` 负责"
- **MAJOR**: `testMultipleConsumersRoundRobin` 测试无效 — 重写为 `testMultipleQueuesSingleConsumer`,诚实测试多队列单消费者场景
- **MAJOR**: `RabbitMQConsumer::subscribe` routingKey 语义混乱 — 绑定改用 `topic` 作为 routingKey(订阅主题 = 路由键),添加 Direct/Fanout/Topic 场景文档
- **MAJOR**: `AmqpCppBackend::consume` 的 `onReceived` 未检查 `m_consuming` 标志 — `stopConsuming` 后仍可能收到消息;添加标志检查
- **MINOR**: CMake `FetchContent_Declare(amqpcpp)` 无 `GIT_SHALLOW` — 添加 `GIT_SHALLOW TRUE` 加速克隆
- 新增回归测试 `testCancelConsumeInCallback`:验证回调中 `cancelConsume` 不再导致 UAF

#### MQ 模块二次审核修复 (P2 收尾 - 并发安全加固)

- **CRITICAL**: `AmqpCppBackend` 持有 `m_mutex` 进入 `QEventLoop` 导致死锁 — `declareExchange`/`declareQueue`/`bindQueue`/`unbindQueue`/`cancelConsume` 均持锁后调用 `QEventLoop::exec()`,心跳定时器 lambda 与 `onReceived`→用户回调→`ack` 均会再次获取 `m_mutex`;重构为锁内仅拷贝 `shared_ptr<channel>`,锁外注册回调+`waitForDone` 等待,新增 `waitForDone` 静态辅助方法统一抽取等待逻辑
- **CRITICAL**: `AmqpCppBackend::connect` 的 `m_channel->onError` 捕获局部变量引用导致 UAF — `onError` 是 channel 级别持久回调(amqpcpp 上游确认),channel 生命周期内可多次触发,connect 返回后局部变量销毁;改用成员变量 `m_channelReady`/`m_channelError`/`m_channelErrorMsg`,lambda 捕获 `this`
- **MAJOR**: `AmqpCppBackend::disconnect` 的 blanket `catch(...)` 违反 ADR-001 — 改为 `catch(const std::exception& e)` + `SC_ERROR` 记录 `e.what()`,保留 `catch(...)` 兜底记录 unknown
- **MAJOR**: `AmqpCppBackend::cancelConsume` 先 `erase` 再 `cancel` 导致状态不一致 — cancel 失败时 `m_consumers` 已删除但 amqpcpp 仍在消费,`onReceived` 仅检查 `m_consuming` 不检查 `m_consumers`,消费者"幽灵化"且不可重试;调整为先 cancel 成功后再 erase,失败时保持状态一致允许重试
- **MINOR**: `InMemoryAmqpBackend::dispatchLoop` 的 `sleep_for(10ms)` 丢失 `m_cv` 通知 — ack/nack 释放配额后 `notify_all` 无法唤醒 sleep 中的线程,造成最多 10ms 延迟;改为 `m_cv.wait_for(10ms, predicate)`,可被 notify 唤醒

#### MQ 模块三次审核修复 (P2 收尾 - 回归修复)

- **CRITICAL**: `InMemoryAmqpBackend::dispatchLoop` 的 `wait_for(10ms, predicate)` 引入忙等死循环 — 谓词与上方 `m_cv.wait` 谓词完全相同(队列非空或停止消费),当 prefetchCount 已满但队列非空时谓词恒为 true,`wait_for` 立即返回不等待 10ms,导致 CPU 100% 忙等(原 `sleep_for` 无此问题);改为无谓词 `m_cv.wait_for(lock, 10ms)`,既可被 ack/nack 的 `notify_all` 提前唤醒,又保证至少让出 CPU 10ms
- **MAJOR**: `AmqpCppBackend::cancelConsume` 的 erase 阶段误删新消费者 — 先 cancel(锁外)后 erase(锁内)期间,若 `consume` 通过 Qt 事件循环重入(invokeMethod 转发)注册新消费者(consumerTag 不同)覆盖 `m_consumers[queue]`,erase 会误删新消费者记录,后续 `cancelConsume` 命中幂等返回无法取消,消息持续投递;erase 前增加 `consumerTag` 匹配检查,仅删除原 consumerTag 的记录

#### v1.7.0 交付前终审修复 (P0 CRITICAL - 全量加固)

- **CRITICAL**: `DiskCache::put`/`putMany` 未调用 `writeAtomically` — 原子写入方法已定义但从未集成,仍直接 `QFile::open+write`,崩溃时数据文件损坏;改为统一调用 `writeAtomically`(临时文件 + rename)
- **CRITICAL**: `DiskCache::put`/`putMany` 未调用 `evictToFitUnlocked` — maxBytes 淘汰方法已定义但从未集成,磁盘无限增长;put 前调用 `evictToFitUnlocked`,空间不足返回 `InternalError`
- **CRITICAL**: `DiskCache::put`/`get`/`contains`/`getMany` 未调用 `touchUnlocked` — LRU 索引永远为空,淘汰策略失效;命中/写入时刷新 LRU 访问时间
- **CRITICAL**: `DiskCache::recoverFromDiskUnlocked` 未重建 LRU 索引 — 重启后 LRU 为空,旧条目无法被淘汰;改为按文件 mtime 重建索引(将 wall clock 转换为 steady_clock 时间点,旧文件 time_point 较小优先淘汰)
- **CRITICAL**: `DiskCache::evictLruUnlocked`/`purgeEntryUnlocked` LRU 索引键语义不一致 — 原设计用原始 key 但 recover 无法重建;改为统一使用文件绝对路径作为索引键,所有路径(get/put/remove/contains/getMany/clear/过期清理)同步更新 LRU
- **CRITICAL**: `MultiLevelCache::remove` 任一层失败仍继续删除其他层 — L1 成功但 L2 失败时,L2 残留 key 会通过 get 回填复活;改为任一层失败(非 NotFound)立即返回错误,不继续删除其他层
- **CRITICAL**: `CodeGenerator::generateCreateTableSql` SQL 标识符未转义 — 直接拼接 `tableName`/`columnName`,SQL 注入风险;新增 `quoteIdentifier` 方法,使用 SQL 标准双引号包裹并转义内部双引号
- **CRITICAL**: `CodeGenerator::generateEntityHeader` C++ 字符串字面量未转义 — `tableName`/`field.name`/`field.columnName`/`field.defaultValue` 直接拼接进 `"..."`,代码注入风险;新增 `escapeCppString` 方法,转义 `\`/`"`/`\n`/`\t`/`\r`
- **CRITICAL**: `HttpTransport::sendRequest` URL 路径注入 — `serviceName`/`methodName` 直接拼接到 URL,可构造恶意路径;改为 `QUrl::toPercentEncoding` 编码
- **CRITICAL**: `HttpTransport::sendRequest` 请求体未携带 `requestId` — 仅序列化 params,服务端无法关联响应;构造 envelope JSON 包含 service/method/requestId/params

#### v1.7.0 全链路终审修复 (状态一致性 + 死代码清理)

- **CRITICAL**: `DiskCache::put`/`putMany` 失败路径 `totalBytes` 状态不一致 — evict/writeAtomically 失败时已减 oldSize 但未恢复,导致 maxBytes 限制永久失效;调整调用顺序为先淘汰、再写入、最后统一修正 totalBytes,失败时不修改任何状态
- **MAJOR**: `HttpTransport::setConnectTimeout` 接口契约违约 — `m_connectTimeout` 设置后从未在 `sendRequest` 中使用,调用者误以为可配置连接超时;移除 `setConnectTimeout` 和 `m_connectTimeout` 成员(QNetworkRequest 无内置连接超时,统一由 `m_readTimeout` 控制)
- **MAJOR**: `DiskCache::purgeEntryUnlocked` 死代码 — `evictLruUnlocked` 重构后自己实现删除逻辑,`purgeEntryUnlocked` 从未被调用,代码重复维护负担;删除该方法和头文件声明

### Changed

#### 架构改进

- 所有新模块遵循 `Result<T>` 错误处理模式
- 线程安全遵循 ADR-005 Level 2 标准
- RAII 资源管理(`SpanGuard`、智能指针)
- 设计模式应用:
  - 装饰器模式:`CachedRepository`
  - 策略模式:`IAmqpBackend`
  - 模板方法模式:`BaseRepository`

### Verified

- Windows MinGW 11.2.0 (Qt-bundled) + Qt 6.5.3 构建: 成功
- 新增模块单元测试覆盖率达标
- `test_observability`: 42 个测试全部通过
- `test_mq`: InMemoryAmqpBackend 全部用例通过
- 二次审核修复后全量测试: **23/23 passed (100%)**, total 4.34 sec
- **终审 P0 修复后全量测试: 23/23 passed (100%)**, total 4.19 sec
- **全链路终审修复后全量测试: 23/23 passed (100%)**, total 8.32 sec
- `AmqpCppBackend` 修改仅语法验证(Windows 下 `ENABLE_RABBITMQ=OFF` 不编译,Linux/macOS 需集成测试验证)

## [1.6.2] - 2026-07-25

### Fixed

- **MAJOR**: `Future::waitForFinished` silently swallowed all exceptions via `catch (...) {}` — now catches `const std::exception& e` and logs via `Logger::instance().error(e.what())`, with fallback `catch (...)` logging "unknown exception"
- **MAJOR**: `Future::onSuccess` silently swallowed user callback exceptions — now logs callback failures via Logger
- **MAJOR**: `Future::executeCallbacks` (success path) silently swallowed callback exceptions — now logs via Logger
- **MAJOR**: `TaskRunner::runAsync` silently swallowed task exceptions — now logs task failures via Logger
- **MAJOR**: `PluginManager::loadAllPlugins` silently swallowed plugin directory loading exceptions — now logs failures via Logger with error message

### Changed

- **MAJOR**: `TypedEventBus::create()` now uses `std::unique_ptr<TypedEventBus>(new TypedEventBus())` -> `std::shared_ptr` conversion for exception safety (equivalent to `make_shared` while keeping constructor private)
- **MAJOR**: `DI Container::bind()`, `bindInstance()`, `bindSingleton()` now return `Result<void>` instead of `void`, returning `Error(ErrorCode::AlreadyExists)` on duplicate registration — enables callers to detect registration conflicts
- `soul_async` library now PUBLIC-links `soul_logging` (future.h includes logger.h)
- `soul_plugin` library now PUBLIC-links `soul_logging` (plugin_manager.cpp uses logger)

### Added

- `TestDI::init()` hook to clear container before each test (avoids AlreadyExists errors from duplicate registrations across tests)

### Verified

- Windows MinGW 11.2.0 (Qt-bundled) + Qt 6.5.3 build: success
- Full ctest suite: **21/21 passed (100%)**, total 3.98 sec

## [1.6.1] - 2026-07-25

### Fixed

- **CRITICAL**: `QueryWrapper::buildUpdateSql` could generate a WHERE-less full-table UPDATE when SoftDelete was disabled and no conditions were supplied — now throws `std::runtime_error` unless `allowFullTableOperation(true)` is explicitly called, preventing accidental mass data mutation
- **CRITICAL**: `QueryWrapper::buildDeleteSql` had the same full-table DELETE risk — same guard applied
- **MAJOR**: `ConnectionPool::acquire` performed `network->connectTo(url)` network IO inside `m_mutex` lock, blocking all other threads during connection establishment — lock is now released during network IO with a placeholder entry reserving capacity
- **MAJOR**: `ConnectionPool::acquire` returned `nullptr` immediately when `maxConnections` was reached, causing cascading failures under burst load — now waits up to `connectionTimeoutMs` via `std::condition_variable`, waking on `release()`
- **MAJOR**: `ConnectionPool` had no RAII wrapper for acquire/release pairs, leaking connections (stuck `inUse=true`) when exceptions were thrown between acquire and release — added `ConnectionGuard` nested class with move semantics and `acquireGuarded()` returning `Result<ConnectionGuard>`
- **MAJOR**: `QueryWrapper::appendConditions` concatenated op and value without space, producing `name =?` instead of `name = ?` — fixed to insert space between op and value clause for non-NULL operators

### Added

- `QueryWrapper::allowFullTableOperation(bool)` escape hatch for intentional full-table operations
- `ConnectionPool::ConnectionGuard` RAII guard (move-only, auto-releases on destruction)
- `ConnectionPool::acquireGuarded(url)` returning `Result<ConnectionGuard>`
- 4 regression tests in `test_orm.cpp`: full-table UPDATE/DELETE rejection + opt-in bypass
- 2 regression tests in `test_network.cpp`: `acquireGuarded` lifecycle + exception-path auto-release
- `test_orm.cpp` main() rewritten to run both `TestSQLiteRepository` and `TestQueryWrapper` suites via `QTest::qExec` (previously only `TestSQLiteRepository` ran due to single `QTEST_MAIN`)

### Changed

- `ConnectionPool::release()` now calls `m_cond.notify_one()` to wake waiting acquirers
- `ConnectionPool::closeAll()` now calls `m_cond.notify_all()` to wake all waiters
- `ConnectionPool` gains `std::condition_variable m_cond` member and `countTotalLocked()` helper

### Verified

- Windows MinGW 11.2.0 (Qt-bundled) + Qt 6.5.3 build: success (16 libraries + 21 test executables)
- Full ctest suite: **21/21 passed (100%)**, total 6.07 sec
  - `test_orm`: 32 passed (9 TestSQLiteRepository + 23 TestQueryWrapper), 0 failed
  - `test_network`: 25 passed (includes 2 new ConnectionPool RAII tests), 0 failed
  - `test_event_bus`: 5 passed, 0 failed
  - `test_core`, `test_plugin`, `test_configuration`, `test_utils`, `test_base`, `test_di`, `test_result`, `test_logger`, `test_storage`, `test_http_request`, `test_typed_event_bus`, `test_cache`, `test_async`, `test_auth`, `test_data`, `test_mq`, `test_ui`, `test_rpc`: all passed

### Note on Toolchain

- Use Qt-bundled MinGW 11.2.0 (`F:/IDE.2/QT/Tools/mingw1120_64/bin/g++.exe`) for local builds to match Qt 6.5.3's ABI. Using newer MinGW (e.g. 14.2.0) causes `libstdc++-6.dll` ABI mismatch and `test_core`/`test_plugin` crash with `0xc0000139`.
- Recommended configure: `cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_CXX_COMPILER="F:/IDE.2/QT/Tools/mingw1120_64/bin/g++.exe" -DQT6_DIR="F:/IDE.2/QT/6.5.3/mingw_64" -DBUILD_TESTS=ON`

## [1.6.0] - 2026-07-25

### Added

- **SoulRPC Framework**: Complete RPC framework for distributed service communication
  - `ISerializer` abstraction with `JsonSerializer` implementation (JSON serialization/deserialization with type-tagged variant support)
  - `IRpcTransport` transport abstraction with `HttpTransport` implementation (HTTP-based RPC using QNetworkAccessManager)
  - `ServiceDispatcher` server-side dispatcher (thread-safe service registration and dispatch)
  - `ClientProxy` client-side proxy (synchronous RPC calls with automatic UUID request ID generation)
  - `IServiceRegistry` / `InMemoryServiceRegistry` service discovery (instance registration and lookup)
  - `LoadBalancer` with round-robin and random selection strategies
  - Full test suite: serialization, dispatch, proxy, registry, load balancing, value types

- **CI/CD Pipeline**: Automated build, test, lint, and release workflows
  - `.github/workflows/ci.yml` — Multi-platform CI (Ubuntu/Windows), build + test + lint + coverage
  - `.github/workflows/lint.yml` — Clang-Tidy + CppCheck static analysis pipeline
  - `.github/workflows/release.yml` — Multi-platform release pipeline on version tags

- **CMake Presets**: `CMakePresets.json` with `default`, `test`, `lint`, `release` configurations
- **Clang-Tidy Configuration**: `.clang-tidy` with LLVM style + modernize/readability/cppcoreguidelines checks
- **Architecture Decision Records**: 5 ADRs documenting key design decisions
  - ADR-001: Error Handling Boundary Rules (bool vs Result<T>)
  - ADR-002: Module Dependency Rules (5-layer architecture)
  - ADR-003: Memory Management Policy (smart pointers + Qt parent-child)
  - ADR-004: ORM Multi-Database Architecture (Strategy pattern)
  - ADR-005: Thread Safety Policy (4-level classification)

### Changed

- **Error Handling Overhaul**: Replaced all 19 blanket `catch (...)` blocks with specific `catch (const std::exception&)` + fallback, preserving diagnostic information
- **Raw Pointer Cleanup**: Replaced 6 raw `new` allocations without parent with `std::unique_ptr` using Qt `deleteLater` deleters
- **Result<T> Adoption**: Converted 5 public APIs from `bool` to `Result<void>` in AuthManager and TaskRunner
- **CMake Integration**: Added `soul_rpc` library with 6 source files, 7 headers, Qt6::Network dependency

### Fixed

- **CRITICAL**: `buildUpdateSql` SET clause placeholders conflicted with WHERE clause for PostgreSQL — fixed with `startIndex` parameter
- **CRITICAL**: `getUpdateBindValues()` missing — added method returning SET + update_time + WHERE values in correct order
- **CRITICAL**: `buildValueClause` placeholder index not cumulative — fixed with `int& index` reference parameter
- **CRITICAL**: `cleanupWidgetAnimations` called `widget->setGraphicsEffect(nullptr)` in `destroyed` handler (UB) — removed widget method call
- **CRITICAL**: `buildUpdateSql` hardcoded `?` and `deleted=0` — now uses `placeholder()` and `SoftDeleteConfig`
- **MAJOR**: 19 `catch (...)` blocks now preserve exception diagnostic info
- **MAJOR**: 6 raw `new` allocations now use RAII with smart pointers
- **MAJOR**: 5 `bool`-returning public APIs now return `Result<void>`

### Removed

- Zero: No features removed. All changes are additive or bug-fix.

## [1.5.1] - 2026-07-25

### Architecture

- **ORM Multi-Database Refactor (MyBatis-Plus Pattern)**: Transformed `SQLiteRepository` into a generic `SqlRepository<T>` with `ISqlDialect` injection, enabling any database to be supported via dialect injection rather than subclass duplication.
  - `ISqlDialect` abstraction: `SQLiteDialect`, `MySqlDialect`, `PostgreSqlDialect` implementations covering placeholder conversion, identifier escaping, LIMIT/OFFSET syntax, type casting, and soft-delete configuration
  - `SqlRepository<T>`: Single repository implementation that works with any SQL dialect via constructor injection
  - `SoftDeleteConfig`: Configurable soft-delete column name and logic values (default: `deleted=0/1`), with `enabled=false` falling back to physical DELETE
  - `BaseRepository<T>`: Default implementations for `findById`, `findAll`, `removeById`, `count()`, `existsById`, `saveBatch`, `findOne` — subclasses only implement 5 core methods (`find`, `save`, `remove`, `count(query)`, `executeSql`)
  - `QueryWrapper`: Dialect-aware SQL generation for LIMIT/OFFSET, placeholders, and soft-delete predicates
  - `SQLiteRepository<T>` preserved as a `typedef` alias for backward compatibility

### Added

- `src/soul/orm/sql_dialect.cpp`: Full implementation of `ISqlDialect::create()` factory with SQLite/MySQL/PostgreSQL dialect classes
- `SoftDeleteConfig` struct in `sql_dialect.h` for configurable soft-delete behavior
- 3 dialect verification tests: PostgreSQL `$1/$2` placeholders, MySQL `LIMIT offset,count` syntax, SQLite `LIMIT..OFFSET` syntax
- 3 QueryWrapper grouping tests: `and_()` combination, `or_()` grouping semantics, mixed AND/OR precedence

### Changed

- `BaseRepository<T>`: Converted `findById`/`findAll`/`removeById`/`count()`/`existsById`/`saveBatch` from pure virtual to default implementations
- `SqlRepository<T>`: Removed 4 redundant method overrides (`findById`, `findAll`, `removeById`, `count()`), delegating to `BaseRepository` defaults
- `generateInsertSql`/`generateUpdateSql`: Replaced hardcoded `?` placeholders with `m_dialect->convertPlaceholder(index)` for PostgreSQL `$1,$2,...` support
- `buildSelectSql`/`buildCountSql`/`buildDeleteSql`: Replaced hardcoded `deleted=0`/`deleted=1` with `SoftDeleteConfig` from dialect
- `QueryWrapper::Condition`: Replaced `isGroupStart`/`isGroupEnd` booleans with `openParens`/`closeParens` counters for correct nested grouping
- `ThreadPool`: Changed `m_threadPool` from `unique_ptr` to `shared_ptr` for thread-safe lifetime management; all methods copy the pointer before use
- `.gitignore`: Added `/*.py`, `/*.ps1`, `/*.bat`, `/*.sh` rules to prevent root-level temp script commits

### Fixed

- **CRITICAL**: `saveBatch` double-processed new entities (set ID + `beforeInsert` then called `save` which routed to UPDATE) — now delegates directly to `save()`
- **CRITICAL**: `QueryWrapper::or_()` set all sub-conditions to OR logic, producing `OR a OR b` instead of `OR (a AND b)` — now only the first condition is marked OR, rest stay AND
- **CRITICAL**: Mixed AND/OR conditions produced incorrect SQL precedence — `hasOr` detection now wraps all conditions in parentheses
- **CRITICAL**: Nested `or_()` caused unbalanced parentheses due to `singleCondGroup` suppression — `openParens`/`closeParens` accumulators handle arbitrary nesting depth
- **CRITICAL**: `ISqlDialect::create()` was declared but never implemented (linker error) — full factory implementation added
- **CRITICAL**: `INSERT`/`UPDATE` SQL hardcoded `?` placeholder, breaking PostgreSQL `$1,$2` — now uses `convertPlaceholder()`
- **MAJOR**: `ThreadPool::shutdown()` held mutex during `waitForDone()`, risking self-deadlock — now copies `shared_ptr` under lock, calls `waitForDone()` outside lock
- **MAJOR**: `DI Container` Scoped instances used `shared_ptr` with default deleter, causing double-free in `disposeScope()` — Scoped instances now use empty deleter `[](T*){}`
- **MAJOR**: `GlassWidget` leaked `QGraphicsBlurEffect` via raw `new` — wrapped in `std::unique_ptr`
- **MAJOR**: `MemoryRepository` lacked mutex protection on all CRUD operations — added `std::mutex` + `std::lock_guard`
- **MAJOR**: `Singleton` base destructor was non-virtual — made `virtual` to prevent UB on subclass deletion
- **MAJOR**: `DbConnectionPool::release()` didn't decrement `m_totalCount` for disconnected drivers — fixed resource tracking
- **MAJOR**: `testRemove` expected `findById` to return soft-deleted record, but `WHERE deleted=0` filters it — corrected assertion to expect `NotFound` error

### Removed

- 53 temporary development scripts (Python/PowerShell) from repository root

## [1.5.0] - 2026-07-24

### Added

- **Data Module Implementation**: Complete data layer with multiple database driver support
  - `DatabaseDriverFactory` with SQLite, MySQL, PostgreSQL implementations
  - `MemoryRepository<T>` - Generic in-memory repository implementation
  - `Transaction` and `ITransactionManager` interfaces
  - `DbConnectionPool` - Database connection pooling with acquire/release lifecycle
- **DI Container Enhancement**: `resolve<T>()` now returns `Result<std::shared_ptr<T>>`
  - Proper error handling for unregistered types, null creators, and invalid lifetimes
  - `SingletonWrapper<T>::get()` updated to return `Result`
- **ThreadPool Lifecycle Management**: `init()`, `shutdown()`, `isInitialized()` methods
  - Lazy initialization with double-checked locking
  - Automatic cleanup on application shutdown
- **Test Suite Expansion**: Added comprehensive test coverage
  - `test_data.cpp`: MemoryRepository CRUD, DatabaseDriverFactory, DbConnectionPool (22 tests)
  - `test_orm.cpp`: QueryWrapper, Entity CRTP, SQLiteRepository CRUD (30 tests)
  - `test_mq.cpp`: Message structure, MQFactory, Interface validation (14 tests)
  - `test_ui.cpp`: Theme management, Style, BaseWidget (14 tests)

### Changed

- **ConnectionPool Naming Conflict Resolution**: Renamed `sc::data::ConnectionPool` to `sc::data::DbConnectionPool`
- **Static Library Export Macros**: Fixed `SC_DI_EXPORT` and `SC_PLUGIN_EXPORT` macros for static lib builds
  - Added `SC_DI_STATIC_LIB` and `SC_PLUGIN_STATIC_LIB` conditions to avoid dllimport in static builds
- **Version Bump**: Updated CMakeLists.txt from 1.3.0 to 1.5.0
- **README.md**: Updated version references from 1.3.0 to 1.5.0

### Fixed

- **Data Module Dependencies**: Added Qt6::Sql dependency to `soul_data` module
- **QSqlRecord Include**: Added missing `#include <QSqlRecord>` in database_driver.cpp
- **Transaction State Tracking**: Replaced non-existent `QSqlDatabase::isTransactionActive()` with manual flag tracking
- **ORM Update Bug**: Fixed `SQLiteRepository::updateInternal()` missing `update_time` parameter binding
- **Plugin Cast Warning**: Suppressed `-Wcast-function-type` for dynamic plugin loading on GCC
- **DI Test State Leakage**: Fixed `testIsRegistered` by clearing container state before assertion

## [1.4.0] - 2026-07-23

### Added

- **Missing Implementation Files**:
  - `database_driver.cpp` - DatabaseDriverFactory implementation
  - `memory_repository.cpp` - MemoryRepository compilation unit
  - `transaction.cpp` - Transaction compilation unit

### Changed

- **CMakeLists.txt**: Added `SOUL_DATA_SOURCES` and linked Qt6::Sql for data module

## [1.3.0] - 2026-07-21

### Added

- **Dependency Injection Container**: `sc::di::Container` with factory function registration pattern
  - Lifetime management: `Transient`, `Singleton`, `Scoped`
  - Thread-safe `resolve()` using Double-Checked Locking Pattern (DCLP) with `std::recursive_mutex`
  - `bind<T>()`, `bindSingleton<T>()`, `bindInstance<T>()` registration APIs
  - `SingletonWrapper<T>` for bridging existing `Singleton<T>` instances
  - `registerSingleton<T>()` for four-phase migration strategy
- **Plugin System**: `sc::plugin::PluginManager` with C-ABI boundary interface
  - Cross-platform dynamic library loading (DLL/SO/DYLIB)
  - `PluginMetadata` specification with ABI/API version compatibility checking
  - `IPlugin` interface with lifecycle management (load → initialize → shutdown → unload)
  - `PluginHandle` with automatic shutdown on destruction
  - Thread-safe plugin operations with deadlock-free `initializeAllPlugins()`/`shutdownAllPlugins()`
- **DI Module**: `soul_di` static library with `SC_DI_EXPORT` macro
- **Plugin Module**: `soul_plugin` static library with `SC_PLUGIN_EXPORT` macro
- **Test Suite**: `test_di.cpp` covering DI-T01 through DI-T11 acceptance criteria

### Changed

- Updated CMakeLists.txt to include `soul_di` and `soul_plugin` modules
- Updated `SoulCoreKit` interface library to link new modules
- Updated install targets to include new modules

### Fixed

- DI container: Singleton shared_ptr deleter design (use-after-free)
- DI container: `resolve()` deadlock with recursive dependency resolution
- DI container: DCLP properly implemented with atomic flag
- Plugin system: `initializeAllPlugins()`/`shutdownAllPlugins()` deadlock
- Plugin system: `getPlugin()` always returning nullptr
- Plugin system: Missing ABI version compatibility check
- Plugin system: `PluginHandle` destructor not ensuring plugin shutdown

## [1.2.0] - 2026-07-20

### Added

- **Network Module Fixes**: Cross-module header inclusion protection
  - Added `#ifndef Q_MOC_RUN` guards for `soul/core/*` includes in network headers
  - Ensured all `SC_NETWORK_EXPORT` classes include `network_global.h`

### Fixed

- MOC preprocessor errors when processing non-Qt class headers
- Missing `network_global.h` includes in multiple network headers:
  `monitor.h`, `reconnect_policy.h`, `retry_policy.h`, `timeout_policy.h`,
  `logging_interceptor.h`, `auth_interceptor.h`, `json_codec.h`,
  `http_client_adapter.h`, `tcp_client_adapter.h`, `ws_client_adapter.h`,
  `mqtt_client_adapter.h`, `bluetooth_client_adapter.h`,
  `serial_port_adapter.h`, `named_pipe_adapter.h`, `http_api.h`
- `HttpApi` class namespace moved from `sc` to `sc::network`
- CI build failures on Ubuntu, macOS, and Windows platforms

## [1.1.0] - 2026-07-14

### Added

- **Protocol-Agnostic Network Layer**: Unified `INetwork` interface supporting HTTP/TCP/WebSocket
- **Network Factory**: `NetworkFactory` for protocol-based instance creation
- **Adapter Pattern**: `HttpClientAdapter`, `TcpClientAdapter`, `WsClientAdapter`
- **Policy Layer**: `RetryPolicy`, `TimeoutPolicy`, `HeartbeatPolicy`
- **Interceptor Chain**: `LoggingInterceptor`, `AuthInterceptor`
- **Codec Layer**: `JsonCodec`, `CodecFactory`
- **Monitor Layer**: `Metrics`, `Monitor` for QPS/RT/成功率 statistics
- **Connection Pool**: `ConnectionPool` with max connections and idle timeout
- **Future Protocol Support**: MQTT, Bluetooth, SerialPort, NamedPipe adapters
- **Result<T> Pattern**: Type-safe error handling
- **Event Bus**: Publish-subscribe event system with Qt signal bridging
- **Async Task Framework**: Thread pool based async execution
- **UI Component Library**: 30+ modern UI components with theme support
- **Configuration Management**: JSON/INI configuration with schema validation
- **Storage Layer**: Memory, file, SQLite storage backends
- **Authentication**: AuthManager, TokenManager, Permission system

### Changed

- Refactored network module to support protocol-agnostic abstraction
- Separated interface (`INetwork`) from signal base (`NetworkBase`) to comply with Qt MOC constraints
- Migrated `RetryPolicy` from network root to `policy/` subdirectory
- Extended `IInterceptor` to support all protocols via `NetworkMessage`

### Deprecated

- `sc::IInterceptor` - Use `sc::network::IInterceptor` instead
- `sc::RetryPolicy` - Use `sc::network::RetryPolicy` instead

### Removed

- Redundant `network.h` header file

### Fixed

- `MetricData::minResponseTime` initialization to `INT64_MAX` for correct statistics
- `HeartbeatPolicy::apply()` empty implementation
- `ConnectionPool` missing `QObject` inheritance for QTimer event loop

## [1.0.0] - 2026-07-14

### Added

- Initial release of SoulCoreKit framework
- Core modules: soul_core, soul_utils, soul_logging
- Network module: HTTP, TCP, WebSocket support
- UI module: Modern Qt Widgets component library
- Async module: Thread pool and task system
- Event module: Event bus with Qt integration
- Storage module: SQLite and memory storage
- Configuration module: JSON/INI config support
- Auth module: Token-based authentication
- Base module: Business base classes
