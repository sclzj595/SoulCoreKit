# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.9.1] - 2026-07-29 (In Progress)

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
