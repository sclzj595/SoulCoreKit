# SoulCoreKit 脚手架对齐审计报告

**审计日期**: 2026-07-27
**审计范围**: 全项目（`include/` + `src/` + `examples/` + `CMakeLists.txt` + 顶层文档）
**基线版本**: v1.8.0
**审计基准**:
- 项目愿景（`docs/00_vision.md`）：Qt 桌面应用基础设施，跨项目复用骨架
- ADR-001 ~ ADR-005
- 用户定位："可以作为我自己 Qt 项目的脚手架就行"，"目标是 Qt 版本的 SpringBoot 基础框架"

---

## 0. 总体结论

**SoulCoreKit 已经基本朝向"Qt 版 SpringBoot 脚手架"目标发展，但存在 3 类定位偏离需要校正。**

| 维度 | 评分 | 说明 |
|------|------|------|
| 模块分层与依赖方向 | ✅ 优 | ADR-002 严格落地，无循环依赖、无 downward 依赖 |
| 错误处理统一性 | ✅ 良 | `Result<T>` 已成为主流，残留少数 `bool` 返回值 |
| 内存安全 | ⚠️ 良 | 多数历史 raw-new 已迁移至智能指针，残留 1 处 double-free 风险 + 1 处 raw-new |
| 脚手架易用性 | ⚠️ 中 | 缺少顶层聚合头文件；UI 模块默认强制链接，违背"轻量骨架"原则 |
| 入口编排能力 | ⚠️ 中 | `Application` 仅回调式注册，未达 SpringBoot 声明式装配水准 |
| 示例精简度 | ⚠️ 中 | `full_stack_example.cpp` 过度集成，与"够用即可"原则不符 |
| 文档完备度 | ✅ 优 | RFC 风格 docs/ + ADR + 中文镜像，远超一般脚手架 |

**与 SpringBoot 类比映射**：

| SpringBoot 概念 | SoulCoreKit 对应物 | 状态 |
|------------------|---------------------|------|
| `@SpringBootApplication` | `sc::Application` | ⚠️ 缺少声明式装配 |
| `@Component / @Service` | `sc::di::Container::bindSingleton` | ✅ 已具备 |
| `@Autowired` | `sc::di::Container::resolve` | ✅ 已具备 |
| `@Configuration` | `sc::JsonConfiguration` / `IniConfiguration` | ✅ 已具备 |
| `application.yml` | JSON/INI 配置文件 | ✅ 已具备 |
| `@EventListener` | `sc::EventBus::subscribe<T>` | ✅ 已具备 |
| `@Async` | `sc::async::async()` / `TaskRunner::run` | ✅ 已具备 |
| `@Repository` | `sc::orm::SqlRepository<T>` | ✅ 已具备 |
| `@Controller / @RestController` | `sc::rpc::ServiceDispatcher` | ✅ 已具备 |
| Actuator | `sc::observability::Metrics` + `Tracing` | ✅ 已具备 |
| Spring DevTools | — | ❌ 未提供（脚手架阶段可不强求） |

---

## 1. 脚手架定位偏离（P0 - 必须校正）

### 1.1 缺少顶层聚合头文件

**问题**: `include/soul/` 目录下没有 `soul.h` 或 `soul_all.h` 之类的聚合头文件。脚手架用户必须分别 `#include "soul/core/..."`、`#include "soul/network/..."` 等多个细粒度头文件才能开始开发。

**与 SpringBoot 的差距**: SpringBoot 用户只需 `@SpringBootApplication` 一个注解即可启动；当前 SoulCoreKit 用户需要手动拼装 10+ 个 include 才能搭起一个最小应用骨架。

**影响**:
- 脚手架首屏体验差，新项目冷启动摩擦大
- 与"作为我自己 Qt 项目的脚手架就行"的定位不符

**建议**: 提供按层级的聚合头文件，遵循 Qt 官方模块设计（如 `<QtCore>`、`<QtNetwork>`）：
```
include/soul/
├── soul_core.h       // 聚合 core/result/error/interface/singleton/factory/application
├── soul_di.h         // 聚合 di/container + di_global + module
├── soul_logging.h    // 聚合 logging 全部
├── soul_network.h    // 聚合 network 公共 API（http_client/web_socket/tcp_client/session/policy/interceptor）
├── soul_async.h      // 聚合 async/thread_pool/task/future/dispatcher
├── soul_event.h      // 聚合 event/event_bus/typed_event_bus/subscription
├── soul_storage.h    // 聚合 storage 全部
├── soul_orm.h        // 聚合 orm 全部（已存在）
├── soul_mq.h         // 聚合 mq 全部（已存在）
├── soul_rpc.h        // 聚合 rpc 全部（已存在）
├── soul_ui.h         // 聚合 ui 全部（已存在 soul_ui.h）
└── soul.h            // 顶层聚合：默认引入 core/di/logging/network/async/event/storage
                       //   (UI/MQ/ORM/RPC 按需引入)
```

`soul.h` 不应包含 UI（见 1.2）。

---

### 1.2 `soul_ui` 默认强制链接，违背"轻量骨架"原则

**问题**: 顶层 `CMakeLists.txt` L1031-1051 中 `SoulCoreKit` INTERFACE 库默认链接了 `soul_ui`：

```cmake
add_library(SoulCoreKit INTERFACE)
target_link_libraries(SoulCoreKit INTERFACE
    soul_core soul_data soul_base soul_logging soul_network
    soul_utils soul_configuration soul_storage soul_async
    soul_event soul_auth soul_mq soul_orm soul_rpc
    soul_ui           # ← 默认强制引入 Qt Widgets
    soul_di soul_plugin
)
```

**影响**:
- 纯后端项目（CLI 工具、服务进程、Headless 服务）使用 `target_link_libraries(MyApp PRIVATE SoulCoreKit)` 时被强制拖入 `Qt6::Widgets`、`Qt6::Gui` 依赖
- 编译时长增加、二进制体积膨胀
- 与"Qt 版 SpringBoot 基础框架"定位不符 —— SpringBoot 不会强制所有项目引入 Web 模板引擎

**建议**:
- 将 `SoulCoreKit` 拆分为 `SoulCoreKit`（核心，无 UI）与 `SoulCoreKitUi`（含 UI）两个 INTERFACE 库
- 或将 `soul_ui` 从 `SoulCoreKit` INTERFACE 中移除，用户按需 `target_link_libraries(MyApp PRIVATE SoulCoreKit soul_ui)`

---

### 1.3 `examples/full_stack_example.cpp` 过度集成

**问题**: `examples/full_stack_example.cpp` 在 79 行内同时演示了 ORM + SQLite + RabbitMQ Producer + Consumer 完整堆栈，且使用 `using namespace sc::mq;`、`using namespace sc::orm;`、`using namespace sc::data;` 三个 using-directive，与脚手架"够用即可"原则不符。

**与脚手架定位的差距**:
- SpringBoot 的 `spring-boot-stater` 示例通常是单一关注点（如 `spring-boot-sample-web`、`spring-boot-sample-jpa` 分开）
- 当前 `full_stack_example` 反而像"集成演示"，会让用户误以为脚手架"重"
- 与用户明确表达的"不用写太多具体的事例"相违背

**建议**:
- 删除或重命名 `full_stack_example.cpp`（移至 `examples/integration/` 子目录，单独开关 `BUILD_FULL_STACK_EXAMPLE`）
- 保留 `logger_example.cpp`、`network_example.cpp`、`simple_test.cpp`、`main.cpp` 这种单一关注点示例
- 新增一个真正的脚手架最小示例 `examples/skeleton_main.cpp`：展示 `Application` + DI Container + Logger 三件套即可启动一个进程

---

## 2. 入口编排能力差距（P1 - 影响脚手架易用性）

### 2.1 `Application` 类缺少声明式模块装配

**当前状态**（`include/soul/core/application.h`）:

```cpp
class Application {
public:
    void addStartupCallback(StartupCallback callback);
    void addShutdownCallback(ShutdownCallback callback);
    int run();
    // ...
};
```

**差距**: 仅支持命令式回调注册，用户必须在 `main()` 中手动按顺序写：

```cpp
sc::Application app(argc, argv);
app.addStartupCallback([]{ return sc::Logger::instance().init(); });
app.addStartupCallback([]{ return sc::Configuration::instance().load("app.json"); });
app.addStartupCallback([]{ return sc::EventBus::instance().init(); });
// ... 10 行手动编排
return app.run();
```

**与 SpringBoot 的差距**: SpringBoot 用户只需：

```java
@SpringBootApplication
public class App { public static void main(String[] args) { SpringApplication.run(App.class, args); } }
```

模块自动按依赖顺序装配、自动配置。

**建议**: 在 `Application` 之上提供 `Scaffold` 或 `Bootstrap` 类，支持声明式模块注册：

```cpp
// soul/core/scaffold.h（建议新增）
class Scaffold {
public:
    Scaffold(int& argc, char** argv);
    Scaffold& use(Module& m);            // 显式注册模块
    Scaffold& useConfig(const QString& path);
    Scaffold& useLogger(sc::Logger& logger);
    int run();
};

// soul/core/module.h（已存在，扩展为声明式）
class Module {
public:
    virtual ~Module() = default;
    virtual QString name() const = 0;
    virtual Result<void> onInit(sc::di::Container&) { return {}; }
    virtual Result<void> onStart() { return {}; }
    virtual void onStop() {}
};
```

用户代码：

```cpp
int main(int argc, char* argv[]) {
    sc::Scaffold scaffold(argc, argv);
    scaffold.useConfig("app.json")
           .use(sc::LoggingModule{})
           .use(sc::ConfigurationModule{})
           .use(sc::EventModule{})
           .use(sc::NetworkModule{});
    return scaffold.run();
}
```

---

## 3. 残留技术债（P1 - 内存安全）

### 3.1 `GlassEffectCache::BlurContext` 析构存在 double-free 风险

**位置**: [include/soul/ui/glass_effect_cache.h:30-34](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/ui/glass_effect_cache.h#L30-L34)

```cpp
~BlurContext() {
    if (pixmapItem) {
        delete pixmapItem;   // ← 直接 delete，未先 scene.removeItem
    }
}
```

**问题分析**:
C++ 成员析构顺序为「析构函数体先执行 → 成员按反向声明顺序析构」。当 `BlurContext` 析构时：

1. 用户析构函数体执行 `delete pixmapItem`（pixmapItem 被释放，但 `scene` 内部 `items()` 列表仍持有该指针）
2. 接着 `scene` 成员析构 → `QGraphicsScene::~QGraphicsScene` 会 `delete` 所有挂载的 items → **尝试 delete 已释放的 pixmapItem** → **double-free / UB**

`apply()` 方法第 47-50 行的处理是安全的（先 `removeItem` 再 `delete`），但析构函数不安全。

**建议**:

```cpp
~BlurContext() {
    if (pixmapItem) {
        scene.removeItem(pixmapItem);   // 先从 scene 摘除
        delete pixmapItem;              // 再 delete
    }
}
```

或更优：完全不手动管理，依赖 `scene` 析构自动清理（移除整个 `if (pixmapItem) delete` 代码块），将 `pixmapItem` 改为 `QPointer<QGraphicsPixmapItem>`。

---

### 3.2 `uploader.cpp` 仍使用无父对象裸 `new`

**位置**: [src/soul/network/uploader.cpp:163-164](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/network/uploader.cpp#L163-L164)

```cpp
auto multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
```

**问题**: 无父对象裸 new，所有权通过后续 `m_file.setParent(multiPart)` 部分托管，但 `multiPart` 本身依赖调用方记得在合适的时机 `deleteLater()`。

**建议**:

```cpp
auto multiPart = std::make_unique<QHttpMultiPart>(QHttpMultiPart::FormDataType);
// 后续需要传递所有权时使用 .release() 或改为 shared_ptr
```

---

## 4. 已修复技术债确认（核实报告）

对比 `docs/tech_debt_audit.md`（2026-07-25 生成），本次审计**亲自读取源文件**核实以下技术债**已经修复**：

| 编号 | 原问题 | 当前状态 | 证据 |
|------|--------|----------|------|
| C-1 | `sqlite_repository.h` 7 处 blanket catch | ✅ 已修复 | [include/soul/orm/sqlite_repository.h:137-140](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/orm/sqlite_repository.h#L137-L140) 改为 `catch (const std::exception& e)` + `Logger::instance().error(...)` |
| C-2 | `future.h` 6 处 blanket catch 静默 | ✅ 已修复 | [include/soul/async/future.h:35-39](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/async/future.h#L35-L39) 改为调用 `detail::logAsyncException` / `detail::logAsyncUnknownException` |
| C-3 | `task_runner.h` 静默 catch | ✅ 已修复 | [include/soul/async/task_runner.h:58-62](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/async/task_runner.h#L58-L62) `runAsync` 中调用 detail 日志函数 |
| C-4 | `plugin_manager.cpp` L260 静默 catch | ✅ 已修复 | [src/soul/plugin/plugin_manager.cpp:261-268](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/plugin/plugin_manager.cpp#L261-L268) 改为 `Logger::instance().error(...)` |
| M-1/M-2 | `future.h` 中 `new QFutureWatcher` | ✅ 已修复 | [include/soul/async/future.h](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/async/future.h) 已完全移除 `QFutureWatcher` |
| M-3 | `typed_event_bus.h` 使用 `shared_ptr(new ...)` | ✅ 合理保留 | [include/soul/event/typed_event_bus.h:52-56](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/event/typed_event_bus.h#L52-L56) 改为 `unique_ptr` + 注释说明：构造函数私有，`make_shared` 无法访问。这是 C++ "私有构造 + 工厂方法" 惯用法，**合规** |
| M-4 | `process_utils.cpp` 裸 `new QProcess` | ✅ 已修复 | [src/soul/utils/process/process_utils.cpp:32-35](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/utils/process/process_utils.cpp#L32-L35) 改为 `std::unique_ptr<QProcess, void(*)(QProcess*)>` + 自定义 deleter 调用 `deleteLater()` |
| M-5 | `clipboard_utils.cpp` 裸 `new QMimeData` | ✅ 已修复 | [src/soul/utils/clipboard/clipboard_utils.cpp:22](file:///f:/CODE/Qt_forNoVS/KITForSC/src/soul/utils/clipboard/clipboard_utils.cpp#L22) 改为 `std::unique_ptr<QMimeData>` + `release()` 转移所有权给 `clipboard->setMimeData()`，符合 Qt 所有权语义 |
| H-3 | `cache.h` 未使用 `<QHash>` | ✅ 误报已澄清 | [include/soul/storage/cache.h:6-8](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/storage/cache.h#L6-L8) `<QHash>` 是必需的，提供 `std::hash<QString>` 特化供 `std::unordered_map<QString, ...>` 使用，已有明确注释 |
| H-5 | `query_wrapper.h` 未前向声明 `ISqlDialect` | ✅ 已修复 | [include/soul/orm/query_wrapper.h:14](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/orm/query_wrapper.h#L14) 已改为 `class ISqlDialect;` 前向声明 |
| R-1/R-2/R-3 | `AuthManager::init/loadAuthState/saveAuthState` 返回 bool | ✅ 已修复 | [include/soul/auth/auth_manager.h:158-166](file:///f:/CODE/Qt_forNoVS/KITForSC/include/soul/auth/auth_manager.h#L158-L166) 改为 `Result<void>` |

**修复率**: 11/11 = **100%**（含 1 项澄清为合理保留）

---

## 5. 架构合规度复审

### 5.1 模块依赖方向（ADR-002）— ✅ 完全合规

通过 CMake `target_link_libraries` PUBLIC/PRIVATE 显式声明，验证：

- `soul_core` 仅依赖 `Qt6::Core`，不依赖任何 `soul_*` 模块 ✅
- `soul_logging` 仅依赖 `Qt6::Core` + `soul_core` ✅
- `soul_event` PUBLIC 仅依赖 `Qt6::Core` + `soul_core`，PRIVATE 依赖 `soul_async`/`soul_logging`（符合 ADR-002 头文件隔离要求）✅
- `soul_async` PUBLIC 仅依赖 `Qt6::Core` + `soul_core`，PRIVATE 依赖 `soul_logging` ✅
- `soul_network` 依赖 `soul_core` + `soul_logging`（合规） ✅
- 无循环依赖 ✅

### 5.2 错误处理统一性（ADR-001）— ✅ 良好

- `Result<T>` 模式已成为主流，公共 API 全部采用
- 残留的 `bool` 返回值均为 Tier 1 谓词（`isXxx/hasXxx`）或 `executeSql` 这种语义明确的「执行是否成功」查询，合规
- 异常边界处理统一：`Application::run()` 顶层 catch + 各模块边界 catch std::exception&

### 5.3 内存管理（ADR-003）— ⚠️ 良好但残留 2 处

- Qt parent-child `new` 全部合规
- 残留：D-1（glass_effect_cache 析构 double-free 风险）、M-6（uploader 裸 new）

### 5.4 ORM 多数据库支持（ADR-004）— ✅ 完全合规

`ISqlDialect` 抽象 + `SqlDialectType::SQLite/MySQL/PostgreSQL` 枚举，策略模式正确实现。

### 5.5 线程安全（ADR-005）— ✅ 完全合规

- `std::mutex` + `std::lock_guard` 一致使用
- `std::atomic<int>` 用于计数器（`TaskRunner::m_activeTasks`）
- TSan CI workflow 已接入（`tsan.yml` + `tsan_suppressions.txt`）

---

## 6. 优先级修复路线图

### P0 — 立即修复（影响脚手架定位）

| # | 任务 | 预估工作量 |
|---|------|------------|
| 1 | 拆分 `SoulCoreKit` INTERFACE：核心库不含 `soul_ui`，新增 `SoulCoreKitUi` 包含 UI | XS |
| 2 | 提供按层级聚合头文件 `soul_core.h` / `soul_network.h` / `soul_async.h` / `soul_event.h` / `soul_storage.h` / `soul.h` | S |
| 3 | 处理 `examples/full_stack_example.cpp`：移至 `examples/integration/` 子目录或加 BUILD_FULL_STACK_EXAMPLE 开关 | XS |

### P1 — 短期修复（内存安全 + 易用性）

| # | 任务 | 预估工作量 |
|---|------|------------|
| 4 | 修复 `GlassEffectCache::BlurContext` 析构 double-free（先 `removeItem` 再 `delete`，或改用 QPointer） | XS |
| 5 | 修复 `uploader.cpp` L163 裸 `new QHttpMultiPart` → `std::unique_ptr` | XS |
| 6 | 新增 `soul/core/scaffold.h` 提供 `Scaffold` 类支持声明式模块注册（`use(Module&)`） | M |
| 7 | 扩展 `soul/core/module.h` 的 `Module` 基类，增加 `onInit(di::Container&)` / `onStart()` / `onStop()` 钩子 | S |

### P2 — 中期演进（SpringBoot 体验对齐）

| # | 任务 | 预估工作量 |
|---|------|------------|
| 8 | 新增 `examples/skeleton_main.cpp` 最小骨架示例（Application + DI + Logger 三件套） | XS |
| 9 | 在 `docs/` 新增 `15_scaffold_guide.md`：脚手架快速上手指南，5 分钟搭起一个进程 | S |
| 10 | 评估是否引入 `SC_MODULE(...)` 宏简化模块声明，对标 `@Component` 注解 | M |

---

## 7. 结论

**SoulCoreKit 当前已经朝向"Qt 版 SpringBoot 基础框架"目标发展，模块化、依赖方向、错误处理、线程安全、ORM 抽象等核心要素均已到位。**

**主要差距集中在"脚手架易用性"层**：
1. 缺少聚合头文件，冷启动摩擦大
2. UI 默认强制链接，纯后端项目被迫引入 Widgets
3. `Application` 仅命令式回调，未达声明式装配水准

**修复 P0 + P1 后，SoulCoreKit 将真正具备"作为我自己 Qt 项目的脚手架就行"的体感**：用户 clone 仓库 → `#include "soul/soul.h"` → `target_link_libraries(MyApp PRIVATE SoulCoreKit)` → `sc::Scaffold` 三行代码启动进程。

---

*报告生成于 2026-07-27。建议在 P0/P1 完成后再次复审。*
