# SoulCoreKit Architecture Specification

## Overview

SoulCoreKit follows a **three-layer modular architecture** with strict dependency rules. The framework is divided into **Foundation** (infrastructure) and **Application** (business architecture) layers, with CS and Web as Application sub-layers.

## Three-Layer Architecture

```
                        SoulCoreKit
                             │
              ┌──────────────┴──────────────┐
              │                             │
         Foundation                    Application
              │                             │
              │                        ┌────┴────┐
              │                        │         │
         Core Layer                   CS        Web
              │                        │         │
              │                        │    QtWebEngine
              │                        │    (预留，未实现)
              │                 Module/Router
              │                 Controller
              │                 Service
              │                 ViewModel
              │                 ErrorHandler
```

### Foundation Layer

**职责**: 基础设施能力，与 UI 无关，与业务无关。所有模块独立于 CS/Web。

| 模块 | 职责 | 关键组件 |
|------|------|----------|
| **Core** | Foundation types | Result, Error, Singleton, Application, Version |
| **Base** | Base classes | BaseObject, BaseManager, BaseRepository, BaseViewModel |
| **Logging** | Logging system | Logger, Sink, Formatter, DailyFileSink |
| **Configuration** | Config management | Config, JsonConfiguration, ConfigSchema |
| **Network** | Network communication | HttpClient, WebSocket, TcpClient, Interceptor |
| **Storage** | Data persistence | IRepository, SqliteDatabase, Cache, Serializer |
| **Async** | Async tasks | ThreadPool, Future/Promise, TaskRunner |
| **Event** | Event bus | EventBus, MessageBus, Subscription |
| **Utils** | Utility functions | JSON, File, String, Crypto, Image |

### Application Layer

**职责**: 业务架构能力，组织 Controller/Service/ViewModel/Route。

**当前目录结构 (v2.5.0)**:

```
include/soul/
├── application/           # 应用上下文与注册表
│   ├── application_context.h
│   ├── service_registry.h
│   └── controller_registry.h
├── cs/                    # CS 架构核心类型 (Controller/Router/Service/ViewModel/Error)
│   ├── cs_module.h / cs_router.h / cs_controller.h
│   ├── cs_service.h / cs_view_model.h
│   ├── cs_error.h / cs_error_handler.h
│   └── cs_data_binding.h / cs_navigation.h / ...
├── ui/                    # UI 组件库
└── web/                   # Web 预留 (README only)
```

> **注意**: `application/` 和 `cs/` 均属于 Application 层。`application/` 包含跨模块的注册表/上下文类型，`cs/` 包含 CS 核心类型。当前扁平结构是 v2.1.0 遗留，未来 v3.0 考虑重组。

## 核心生命周期

### 启动链路

```
Application
    │
    ▼
ApplicationContext::create()
    │
    ├── loadModules()          ← 扫描/注册 CsModule
    ├── registerServices()     ← ServiceRegistry 注册所有 CsService
    ├── initializeServices()   ← 对标 @PostConstruct
    ├── registerControllers()  ← ControllerRegistry 注册所有 CsController
    ├── registerRoutes()       ← CsRouter 构建路由表
    ├── connectSignals()       ← Controller ↔ Router ↔ ErrorHandler
    └── Application Ready
```

### 请求链路

```
User Action → CsRouter::navigate → RouteMatcher::match
    → CsController::dispatch (强类型优先)
    → CsService::execute → Repository
    → Result<T> → CsViewModel → Qt Widgets
```

### 停止链路

```
Application::shutdown() → ApplicationContext::close()
    ├── disconnectSignals()
    ├── shutdownServices()    ← 对标 @PreDestroy
    ├── unregisterRoutes()
    ├── disposeControllers()
    └── disposeServices()
```

## Module Dependency Graph

### Allowed Dependencies

```
Application Layer:
  CsRouter ──→ CsController ──→ CsService ──→ Storage (Foundation)
  CsViewModel ──→ CsController
  CsErrorHandler ──→ CsController
  ApplicationContext ──→ CsModule, CsRouter, ServiceRegistry, ControllerRegistry

Foundation Layer:
  Core (Qt Core only, no internal dependencies)
  Base ──→ Core
  Logging ──→ Core
  Configuration ──→ Core, Utils
  Network ──→ Core, Logging
  Storage ──→ Core
  Async ──→ Core, Logging
  Event ──→ Core
  Utils ──→ Core
```

### Prohibited Dependencies

- **Foundation** must NOT depend on Application
- **Core** must NOT depend on any other module
- **Service** must NOT depend on UI (Controller, ViewModel, Widget)
- **ViewModel** must NOT depend on Service or Repository (通过 Controller 访问)
- **Controller** must NOT depend on Repository (通过 Service 访问)
- **No circular dependencies** allowed between any modules

## Module Responsibility Matrix

| Module | Layer | Responsibility | Dependencies | Thread Safety |
|--------|-------|---------------|--------------|---------------|
| **Core** | Foundation | Foundation types | Qt Core | Yes |
| **Base** | Foundation | Base classes | Core | Yes |
| **Logging** | Foundation | Logging system | Core | Yes |
| **Configuration** | Foundation | Config management | Core, Utils | Yes |
| **Network** | Foundation | Network communication | Core, Logging | Yes |
| **Storage** | Foundation | Data persistence | Core | Yes |
| **Async** | Foundation | Async tasks | Core, Logging | Yes |
| **Event** | Foundation | Event bus | Core | Yes |
| **Utils** | Foundation | Utility functions | Core | Yes |
| **CsModule** | Application | Module registration | CsController, CsService | GUI Thread |
| **CsRouter** | Application | Route table + navigation | CsController, Core | GUI Thread |
| **CsController** | Application | Request dispatch | CsService, Core | GUI Thread |
| **CsService** | Application | Business logic | Storage, Core | Yes |
| **CsViewModel** | Application | UI State management | CsController, Core | GUI Thread |
| **CsErrorHandler** | Application | Global error handling | CsController, Core | GUI Thread |

## Key Design Patterns

### 1. ApplicationContext — 轻量级应用上下文

对标 Spring 的 `ApplicationContext`，但**不是巨型 IoC 容器**。仅协调模块注册、服务生命周期、路由构建。

```cpp
class ApplicationContext {
public:
    static ApplicationContext& instance();
    void initialize();   // 加载模块、注册服务、构建路由
    void shutdown();     // 停止服务、清理资源
    void registerModule(std::unique_ptr<CsModule> module);
    ServiceRegistry& serviceRegistry();
    CsRouter& router();
};
```

### 2. ViewModel → Controller → Service → Repository 分层

严格的单向依赖，禁止越层访问：

```
View → ViewModel → Controller → Service → Repository
```

- ViewModel 不直接访问 Service 或 Repository
- Controller 不直接访问 Repository
- Service 不依赖 UI

### 3. Controller 强类型分发

```cpp
// ✅ 推荐: 成员函数指针（编译期类型安全）
route("list", &UserController::listUsers);

// ⚠️ 兼容: 字符串（QML 动态调用场景）
route("list", "listUsers");
```

### 4. CS 与 Web 共享业务 Service

```
                  UserService
                      ▲
            ┌─────────┴─────────┐
       CsController        Web Adapter
```

业务 Service 不感知自己是 CS 还是 Web 调用。

### 5. Singleton with Controlled Lifecycle

```cpp
class Theme : public Singleton<Theme> {
    friend class Singleton<Theme>;
public:
    void init();
    void shutdown();
private:
    Theme() = default;
};
```

**单例模式适用场景**:
- 跨模块的全局横切关注点（如 `CsErrorHandler` 错误处理、`Theme` 主题管理）
- 生命周期与 Application 绑定，通过 `init()`/`shutdown()` 控制
- 不适合用单例的场景：业务 Service（应通过 DI 容器管理）、数据 Repository（应通过 ApplicationContext 获取）

**注意**: `CsErrorHandler` 使用传统 Meyer's 单例而非 DI 容器管理，这是有意为之。作为全局错误处理器，它属于横切关注点，不参与业务依赖注入链。`CsRouter` 通过构造函数注入 `CsErrorHandler&` 引用（而非直接调用单例），`ApplicationContext` 提供 `setErrorHandler()` 方法支持测试注入。

## Architecture Constraints

1. **Foundation 不依赖 Application**: 基础设施模块独立于业务架构
2. **Application 依赖 Foundation**: Controller/Service 使用基础设施
3. **Controller 不直接依赖 Repository**: 通过 Service 访问数据
4. **ViewModel 不直接依赖 Service**: 通过 Controller 获取数据
5. **Service 不依赖 UI**: 业务逻辑与表现层解耦
6. **No UI in Foundation**: Network, Storage, Event 模块不得包含 UI 代码
7. **No Business Logic in View**: UI 组件仅处理表现，不包含业务逻辑

## Anti-Patterns to Avoid

- **Manager 大总管**: 避免单个类管理所有对象（如 `UserModule` 持有 `UserService*` + `UserRepository*` + `UserController*` + `UserViewModel*`）
- **ViewModel 越层访问**: ViewModel 通过 Controller 获取数据，不直接访问 Service/Repository/数据库
- **Web 提前架构污染**: Web 模块仅预留 README.md，不提前实现 WebController/WebRouter
- **字符串分发泛滥**: Controller 分发优先使用成员函数指针，字符串仅作回退

## Known Technical Debt

### TD-001: soul_application ↔ soul_cs 循环依赖

**现状**: `soul_application` 定义为 INTERFACE 库（仅头文件），其实现文件（`service_registry.cpp`、`controller_registry.cpp`、`application_context.cpp`）归入 `soul_cs` 编译。`service_registry.h` 包含 `soul/cs/cs_service.h`，`controller_registry.h` 包含 `soul/cs/cs_controller.h`，形成 `application/` 层依赖 `cs/` 层的反向依赖。

**影响**:
- `soul_application` 不能独立使用，必须与 `soul_cs` 同时链接
- 无法单独测试 Application 层组件（必须编译整个 `soul_cs`）

**解决方向** (v3.0):
- 提取 `ILifecycleManaged` 接口到 `soul/core/`，解耦 ServiceRegistry 对 CsService 的依赖
- 提取 `IRouteHandler` 接口到 `soul/application/`，解耦 ControllerRegistry 对 CsController 的依赖
- 将 `soul_application` 从 INTERFACE 改为 STATIC 库，实现文件独立编译

### TD-002: CsController 继承 QWidget（通过 sc::Page）

**现状**: `CsController` 继承 `sc::Page` → `sc::ui::BaseWidget` → `QWidget`。Controller 同时承担路由分发、页面生命周期和 UI 渲染职责。

**理由**: 在桌面 CS 架构中，路由分发和页面生命周期是天然耦合的——导航到路径意味着创建 Controller 实例并推入页面栈。这是有意简化，而非架构疏忽。

**解决方向** (v3.0):
- 提取 `ICsRouteHandler` 纯虚接口，分离路由分发与 QWidget 生命周期
- Controller 实现 `ICsRouteHandler`，通过组合（而非继承）持有 Page 实例