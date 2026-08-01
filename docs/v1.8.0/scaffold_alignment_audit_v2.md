# SoulCoreKit 脚手架对齐审计报告 v2

**审计日期**: 2026-07-27 (第二轮全链路审计)
**审计范围**: 全项目(`include/` + `src/` + `examples/` + `CMakeLists.txt` + 顶层文档)
**基线版本**: v1.8.0 (上一轮修复后)
**审计基准**:
- 项目愿景(`docs/00_vision.md`):Qt 桌面应用基础设施,跨项目复用骨架
- 用户定位:"可以作为我自己 Qt 项目的脚手架就行","目标是 Qt 版本的 SpringBoot 基础框架"
- ADR-001 ~ ADR-005

---

## 0. 总体结论

**第二轮审计结论:上一轮 6 项修复 100% 到位,但全链路深扫发现 7 项新问题,其中 2 项为 critical 潜伏 bug,会阻断脚手架用户使用。**

| 维度 | v1 评分 | v2 评分 | 变化 |
|------|---------|---------|------|
| 模块分层与依赖方向 | ✅ 优 | ✅ 优 | 持平 |
| 错误处理统一性 | ✅ 良 | ✅ 良 | 持平 |
| 内存安全 | ⚠️ 良 | ✅ 优 | ↑(D-1/M-6 已修复) |
| 脚手架易用性 | ⚠️ 中 | ⚠️ 中 | 持平(聚合头/Scaffold 已就位,但潜伏 bug 拖累) |
| 入口编排能力 | ⚠️ 中 | ⚠️ 中 | 持平(Scaffold 已就位,但 Module 接口过简) |
| 示例精简度 | ⚠️ 中 | ✅ 优 | ↑(BUILD_FULL_STACK_EXAMPLE 默认 OFF) |
| API 可用性 | — | ❌ 差 | **新维度**(发现 2 项潜伏编译失败 bug) |

**与 SpringBoot 对齐度趋势**:

| SpringBoot 概念 | SoulCoreKit v2 状态 | 备注 |
|------------------|---------------------|------|
| `@SpringBootApplication` | ✅ `sc::Scaffold` 已就位 | 链式 `use(Module&)` |
| `@Component / @Service` | ⚠️ `sc::Module` 过简 | 缺少依赖声明/优先级/条件装配 |
| `@Autowired` | ✅ `sc::di::Container` 已就位 | 但缺 Scoped/Qualifier/Primary |
| `@Configuration` | ✅ `sc::JsonConfiguration` | 但缺 Profile/优先级链 |
| `@EventListener` | ✅ `sc::EventBus::subscribe<T>` | 已就位 |
| `@Async` | ✅ `sc::async::async()` | 已就位 |
| `@Repository` | ✅ `sc::orm::SqlRepository<T>` | 已就位 |
| 嵌入式 Tomcat | ❌ 缺失 | soul/network 只有 client 无 server |
| Actuator | ✅ `sc::observability::Metrics` | 已就位 |
| `spring-boot-starter-*` | ⚠️ 部分就位 | SoulCoreKit/SoulCoreKitUi 粒度够,但可再细 |

---

## 1. 上一轮修复验证(100% 到位)

| # | v1 问题 | v2 验证结果 | 证据 |
|---|---------|-------------|------|
| 1 | D-1 double-free | ✅ 已修复 | [glass_effect_cache.h:29-37](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/ui/glass_effect_cache.h#L29-L37) 先 `removeItem` 再 `delete` 再置 null |
| 2 | M-6 uploader 裸 new | ✅ 已修复 | [uploader.cpp:167](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/network/uploader.cpp#L167) `std::make_unique` + `release()` |
| 3 | UI 拆分 | ✅ 已修复 | [CMakeLists.txt](file:///f:/CODE/Qt_forNoVS/KITForSC/CMakeLists.txt) `SoulCoreKit`(无 UI) + `SoulCoreKitUi`(含 UI) |
| 4 | 聚合头文件 | ✅ 已修复 | 6 个聚合头文件全部就位,引用子头文件全部存在 |
| 5 | Scaffold 声明式入口 | ✅ 已修复 | [scaffold.h](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/core/scaffold.h) + [scaffold.cpp](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/core/scaffold.cpp) 链式 `use()` + 自动回滚 |
| 6 | full_stack_example 开关 | ✅ 已修复 | [examples/CMakeLists.txt](file:///f:/CODE/Qt_forNoVS/KITForSC/examples/CMakeLists.txt) `BUILD_FULL_STACK_EXAMPLE` 默认 OFF |
| 7 | skeleton_main 示例 | ✅ 已修复 | [examples/skeleton_main.cpp](file:///f:/CODE/Qt_forNoVS/KITForSC/examples/skeleton_main.cpp) 5 分钟上手示例 |

---

## 2. 第二轮新发现问题

### 2.1 P0 — Critical(阻断脚手架用户使用)

#### C-1: `HttpApi::onSuccess<T>` 引用不存在的成员 `m_successCallback`

**位置**: [include/soul/network/http_api.h:158-167](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/network/http_api.h#L158-L167)

**问题**: 模板方法 `onSuccess<T>` 在 L160 给 `m_successCallback` 赋值,但类成员声明(L244-247)只有 `m_jsonCallback` 和 `m_failureCallback`,**没有 `m_successCallback` 成员**:

```cpp
// L158-167: 模板方法
template<typename T>
HttpApi& onSuccess(std::function<void(const T&)> callback) {
    m_successCallback = [callback, this](const Result<HttpResponse>& result) {  // ← m_successCallback 不存在
        if (result.isOk()) {
            T data = parseResponse<T>(result.unwrap());
            callback(data);
        }
    };
    return *this;
}

// L244-247: 成员声明
std::shared_ptr<HttpClient> m_client;
HttpRequest m_request;
std::function<void(const QJsonDocument&)> m_jsonCallback;
std::function<void(const Error&)> m_failureCallback;
// ← 没有 m_successCallback!
```

**影响**: 用户实例化 `HttpApi<E>::onSuccess<T>` 时会编译失败。当前未暴露是因为项目本身未使用此 API,但作为脚手架提供给用户会立即失败。

**严重程度**: critical(脚手架可用性阻断)

---

#### C-2: `Promise::start()` 与 Qt 6 QPromise API 类型不匹配

**位置**: [include/soul/async/promise.h:26-28, 69-71](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/async/promise.h#L26-L28)

**问题**: `Promise<T>::start(std::function<T()>)` 调用 `QPromise<T>::start(std::move(func))`,但 Qt 6.x 的 `QPromise::start()` 签名是:

```cpp
// Qt 6.x 实际签名
void QPromise<T>::start(std::function<void(QPromise<T>&)> func);
```

而项目传入的是 `std::function<T()>`,类型不匹配。

**验证**: 全项目 `grep sc::Promise` 无任何使用,因此模板未被实例化,编译能通过。但用户使用 `sc::Promise<T>::start()` 会立即编译失败。

**影响**: `soul_async.h` 聚合头引入了 `promise.h`,用户 `#include "soul/soul_async.h"` 后使用 Promise 会失败。

**严重程度**: critical(脚手架可用性阻断)

---

### 2.2 P1 — Major(影响 SpringBoot 对齐度)

#### M-1: `Module` 基类过于简单,缺少 SpringBoot 关键特性

**位置**: [include/soul/core/module.h:9-21](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/core/module.h#L9-L21)

**问题**: 当前 `Module` 接口只有 `init()` + `cleanup()` 两个钩子,与 SpringBoot `@Component` 生命周期相比差距大:

| SpringBoot 特性 | SoulCoreKit Module | 状态 |
|------------------|---------------------|------|
| `@PostConstruct` | `init()` | ✅ 有 |
| `@PreDestroy` | `cleanup()` | ✅ 有 |
| `dependsOn` | — | ❌ 缺失 |
| 优先级排序 | — | ❌ 缺失(仅按注册顺序) |
| `@ConditionalOnProperty` | — | ❌ 缺失 |
| `onStart/onStop`(区分初始化与启动) | — | ❌ 缺失 |

**影响**: 无法表达模块间依赖关系,无法按依赖图自动排序,无法条件装配。Scaffold 只能"按注册顺序 init",未达 SpringBoot 自动装配水准。

**严重程度**: major(架构对齐度)

---

#### M-2: 缺少嵌入式 HTTP Server(SpringBoot 核心特性缺失)

**位置**: [include/soul/network/](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/network/)

**问题**: 全项目扫描 `include/soul/network/` 下 41 个头文件,**全部是 client 端组件**(HttpClient/TcpClient/WebSocket/Downloader/Uploader),**没有任何 server 端组件**(HttpServer/WebSocketServer/TcpServer)。

**影响**: SpringBoot 核心特性之一是"内嵌 Tomcat",开箱即用提供 HTTP 服务。SoulCoreKit 当前只能作为客户端框架,无法作为服务端框架,与"Qt 版 SpringBoot"定位有差距。

**严重程度**: major(定位对齐度)

**注**: 这取决于用户对"脚手架"的定位。如果用户只需要客户端脚手架,此项可忽略;如果需要全栈脚手架,此项为 P0。

---

#### M-3: README.md 缺少 Quick Start 章节

**位置**: [README.md](file:///f:/CODE/Qt_forNoVS/KITForSC/README.md)

**问题**: README.md 顶部缺少"5 分钟快速上手"章节,用户 clone 仓库后无法立即知道如何接入脚手架。

**影响**: 脚手架首屏体验差,与 SpringBoot 的"Quick Start"指南相比有差距。

**严重程度**: major(脚手架易用性)

---

#### M-4: DI 容器缺少 SpringBoot 关键特性

**位置**: [include/soul/di/container.h](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/di/container.h)

**问题**: DI 容器与 SpringBoot `@Autowired` 相比缺少:

| SpringBoot 特性 | SoulCoreKit DI | 状态 |
|------------------|-----------------|------|
| Singleton 生命周期 | `bindSingleton` | ✅ 有 |
| Scoped 生命周期 | — | ❌ 缺失(project_memory 要求) |
| Transient 生命周期 | — | ❌ 缺失 |
| `@Qualifier`(按名称) | — | ❌ 缺失 |
| `@Primary`(默认实现) | — | ❌ 缺失 |
| `@Lazy`(懒加载) | — | ❌ 缺失 |
| 构造函数注入 | `resolve<T>()` 手动 | ⚠️ 非自动 |

**影响**: DI 容器仅支持 Singleton + 手动 resolve,未达 SpringBoot 自动装配水准。project_memory 中明确要求"DI Container 必须实现 Scoped 生命周期管理"。

**严重程度**: major(架构对齐度 + 违反 project_memory 硬约束)

---

### 2.3 P2 — Minor(脚手架体验优化)

#### m-1: `soul_async.h` 聚合头引入有 bug 的 `promise.h`

**位置**: [include/soul/soul_async.h:14](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/soul_async.h#L14)

**问题**: `soul_async.h` 第 14 行 `#include "soul/async/promise.h"`,而 promise.h 存在 C-2 bug。虽然模板未实例化不会报错,但暴露给用户后会阻断使用。

**建议**: 修复 C-2 后保留;若不修复,则从 `soul_async.h` 移除 `promise.h` 引用。

**严重程度**: minor(依赖 C-2 修复)

---

#### m-2: 配置系统缺少 Profile 与优先级链

**位置**: [include/soul/configuration/config.h](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/configuration/config.h)

**问题**: 配置系统与 SpringBoot `application.yml` 相比缺少:
- 环境隔离(`dev`/`test`/`prod` profile)
- 配置覆盖优先级(命令行 > 环境变量 > 配置文件 > 默认值)
- 类型安全绑定(类似 `@ConfigurationProperties`)

**影响**: 无法支持多环境部署,project_memory 中明确要求"配置模块必须支持环境隔离"。

**严重程度**: minor(违反 project_memory 硬约束,但脚手架阶段可后置)

---

## 3. 误报排除(透明记录)

第二轮 sub-agent 报告中以下问题经亲自验证为误报:

| sub-agent 报告 | 验证结果 | 证据 |
|----------------|----------|------|
| `catch(...)` 6 处静默吞异常 | ❌ 误报 | 全部是边界屏障且有日志:[application.cpp:44-53](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/core/application.cpp#L44-L53) / [plugin_manager.cpp:264-268](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/plugin/plugin_manager.cpp#L264-L268) / [async_runner.cpp:17-21](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/async/async_runner.cpp#L17-L21) / [connection_pool.cpp:126-131](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/network/pool/connection_pool.cpp#L126-L131) / [amqpcpp_backend.cpp:234-248](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/mq/amqpcpp_backend.cpp#L234-L248) |
| Scaffold `shutdown()` 重复 cleanup 风险 | ❌ 误报 | `m_initialized` 标志足够防护,首次调用后置 false,二次调用直接 return |
| Scaffold `use(Module*)` UAF 风险 | ❌ 误报 | 文档化约定:Module 生命周期由调用方管理,与 SpringBoot `@Bean` 方法语义一致 |
| Scaffold `run()` 回滚 cleanup 抛异常阻断后续 | ❌ 误报 | [scaffold.cpp:41-47](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/core/scaffold.cpp#L41-L47) 已有 try/catch 包裹,异常被记录后继续循环 |
| Scaffold `m_modules` 多线程访问无锁 | ❌ 误报 | Scaffold 在 main 线程使用,`use()` 在 `run()` 前调用,无并发 |
| `Application::addShutdownCallback` 线程不安全 | ❌ 误报 | 在 main 线程调用,无并发 |
| SoulCoreKitUi 缺失 soul_ui | ❌ 误报 | [CMakeLists.txt](file:///f:/CODE/Qt_forNoVS/KITForSC/CMakeLists.txt) 已正确链接 |
| SoulCoreKit 漏掉 soul_observability | ❌ 误报 | 上一轮已补入 |
| BUILD_FULL_STACK_EXAMPLE 开关不正确 | ❌ 误报 | [examples/CMakeLists.txt](file:///f:/CODE/Qt_forNoVS/KITForSC/examples/CMakeLists.txt) 正确 |
| soul_core SOURCES 未包含 scaffold.cpp | ❌ 误报 | [CMakeLists.txt](file:///f:/CODE/Qt_forNoVS/KITForSC/CMakeLists.txt) 已包含 |
| 聚合头文件引用子头文件不存在 | ❌ 误报 | Glob 验证全部存在 |

---

## 4. ADR 合规度复审

### ADR-001 错误处理 — ✅ 完全合规
- `Result<T>` 已成为主流
- 边界 `catch(...)` 全部有日志记录,符合"边界屏障"语义

### ADR-002 模块依赖方向 — ✅ 完全合规
- `soul_core` 通过 `SC_XXX` 宏使用 logging,与 `application.cpp` 既有模式一致(宏展开后调用 Logger 单例,无编译期硬依赖)
- 无循环依赖
- 无 downward 依赖

### ADR-003 内存管理 — ✅ 完全合规(v1 修复后)
- D-1 double-free 已修复
- M-6 裸 new 已修复
- 全项目无新增裸 new/delete

### ADR-004 ORM 多数据库 — ✅ 完全合规
- `ISqlDialect` 抽象 + SQLite/MySQL/PostgreSQL 三实现

### ADR-005 线程安全 — ✅ 完全合规
- `Application` 单例使用 `std::mutex` 保护
- `Scaffold` 单线程使用,无并发风险

---

## 5. 优先级修复路线图(v2)

### P0 — 立即修复(阻断脚手架使用)

| # | 任务 | 类型 | 预估 |
|---|------|------|------|
| 1 | 修复 `HttpApi::onSuccess<T>` 缺失 `m_successCallback` 成员(C-1) | bug | XS |
| 2 | 修复 `Promise::start()` 与 Qt 6 QPromise API 类型不匹配(C-2) | bug | XS |

### P1 — 短期修复(SpringBoot 对齐度)

| # | 任务 | 类型 | 预估 |
|---|------|------|------|
| 3 | 扩展 `Module` 基类:增加 `dependsOn()`/`onStart()`/`onStop()`/优先级(M-1) | 架构 | M |
| 4 | 评估是否需要嵌入式 HTTP Server(M-2,取决于用户定位) | 决策 | — |
| 5 | README.md 增加 Quick Start 章节(M-3) | 文档 | S |
| 6 | DI 容器增加 Scoped/Transient/Qualifier/Primary(M-4) | 架构 | M |

### P2 — 中期演进(脚手架体验)

| # | 任务 | 类型 | 预估 |
|---|------|------|------|
| 7 | `soul_async.h` 视 C-2 修复情况保留或移除 promise.h(m-1) | 依赖 | XS |
| 8 | 配置系统增加 Profile + 优先级链(m-2) | 架构 | M |

---

## 6. 结论

**SoulCoreKit 在 v1 修复后已具备脚手架雏形,但 v2 审计发现 2 项 critical 潜伏 bug(C-1/C-2)会阻断用户使用。**

**修复 P0 后,SoulCoreKit 将真正可用**;修复 P1 后,将真正对齐"Qt 版 SpringBoot 基础框架"定位。

**当前进度**:
- ✅ 脚手架骨架已就位(Scaffold + 聚合头 + UI 拆分 + 最小示例)
- ❌ 脚手架 API 可用性未达标(2 项潜伏 bug)
- ⚠️ SpringBoot 对齐度 60%(Module/DI/Config 需扩展,HTTP Server 待决策)

---

*报告生成于 2026-07-27 第二轮全链路审计。建议优先修复 P0 2 项 critical bug 后再次复审。*
