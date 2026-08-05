# SoulCoreKit Module Specification

## Overview

This document specifies the responsibilities, interfaces, and constraints for each module in SoulCoreKit.

---

## Module: Core

### Responsibility
Foundation types and utilities with no third-party dependencies beyond Qt Core.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `Result<T>` | Rust-style result type: Ok(T) / Err(Error), supports chaining | Implemented |
| `Error` | Unified error code + message + nested error support | Implemented |
| `IInterface` | Pure virtual interface base class for RTTI | Implemented |
| `IFactory<T>` | Abstract factory interface for creating objects by name | Implemented |
| `Singleton<T>` | Thread-safe singleton template with controlled lifecycle | Implemented |
| `Application` | Wraps QApplication, provides lifecycle management | Implemented |
| `Platform` | Runtime OS/architecture/environment detection | Implemented |
| `Environment` | Environment variables, command-line parsing, working directory | Implemented |
| `Version` | SemVer 2.0 parsing and comparison | Implemented |
| `Uuid` | UUID generation/parsing (extends QUuid) | Implemented |
| `Time` | Timestamps, timers, timezone conversion, formatting | Implemented |

### Dependencies
- Qt Core only

### Thread Safety
- All components must be thread-safe

---

## Module: Base

### Responsibility
Base classes for all business modules.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `BaseObject` | Inherits QObject, provides debug name, object tree management | Implemented |
| `BaseManager` | Manager base class (singleton or global access) with init/shutdown | Implemented |
| `BaseService` | Service base class (business logic container, DI ready) | Implemented |
| `BaseRepository` | Repository base class (配合 Storage 使用) | Implemented |
| `BaseWidget` | UI widget base class (integrates Style system) | Implemented |
| `BaseDialog` | Dialog base class (unified layout, buttons, event handling) | Implemented |
| `BaseViewModel` | MVVM ViewModel base class (data binding ready) | Implemented |

### Dependencies
- Core

### Thread Safety
- Thread-safe

---

## Module: Logging

### Responsibility
Industrial-grade logging system with sink architecture.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `Logger` | Log facade: debug()/info()/warn()/error()/fatal() | Implemented |
| `LogLevel` | Enumeration: Trace/Debug/Info/Warn/Error/Fatal | Implemented |
| `ISink` | Log output interface for custom extension | Implemented |
| `ConsoleSink` | Console output with color support | Implemented |
| `FileSink` | File output with size-based rotation | Implemented |
| `DailyFileSink` | Daily file rotation | Implemented |
| `CallbackSink` | Callback output for UI display or remote reporting | Implemented |
| `CompositeSink` | Combines multiple sinks | Implemented |
| `LogFormatter` | Formats log: time/level/file/line/message | Implemented |

### Dependencies
- Core

### Thread Safety
- Thread-safe

### Usage Example

```cpp
SC_INFO("Network") << "Connected to server " << url;
SC_WARN("Storage") << "Cache miss for key " << key;
```

---

## Module: Configuration

### Responsibility
Layered configuration management with multi-file support and hot-reload.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `Config` | Configuration entry point: load/save/watch changes | Implemented |
| `IConfiguration` | Configuration source interface | Implemented |
| `JsonConfiguration` | JSON file configuration source | Implemented |
| `IniConfiguration` | INI file configuration source | Implemented |
| `ConfigSchema` | Configuration schema definition (type validation, defaults) | Implemented |

### Dependencies
- Core
- Utils

### Thread Safety
- Thread-safe

### Recommended Config Directory

```
config/
  app.json          # Application base config
  network.json      # Network (timeout/retry/proxy)
  ui.json           # UI (font/size/default theme)
  theme.json        # Theme (colors/corners/spacing)
  logging.json      # Logging (level/sink config)
  [env]/            # Environment overrides (dev/prod)
```

---

## Module: Network

### Responsibility
Complete network communication stack from low-level connections to business API.

### Architecture

```
INetwork (Pure Virtual Interface)
    ├── HttpClientAdapter
    │       └── HttpClient
    ├── TcpClientAdapter
    │       └── TcpClient
    └── WebSocketAdapter
            └── WebSocket
NetworkBase (QObject Signal Base)
    ├── connected() / disconnected()
    ├── received(NetworkMessage)
    ├── error(NetworkError)
    ├── progress(current, total)
    ├── timeout()
    └── reconnect(attempt)
```

### Components

#### Core Layer

| Component | Description | Status |
|-----------|-------------|--------|
| `INetwork` | Protocol-agnostic network interface (connect/disconnect/send/state/policy/interceptor) | Implemented |
| `NetworkBase` | Qt signal base class with unified event signals | Implemented |
| `NetworkMessage` | Protocol-neutral message model (api/payload/metadata/statusCode/message/duration/error) | Implemented |
| `NetworkState` | Unified lifecycle state enum (Created/Connecting/Connected/Working/Reconnecting/Disconnected/Destroyed) | Implemented |
| `NetworkError` | Network error classification and mapping | Implemented |

#### Adapter Layer

| Component | Description | Status |
|-----------|-------------|--------|
| `HttpClientAdapter` | Adapts HttpClient to INetwork interface | Implemented |
| `TcpClientAdapter` | Adapts TcpClient to INetwork interface | Implemented |
| `WebSocketAdapter` | Adapts WebSocket to INetwork interface | Implemented |

#### Policy Layer

| Component | Description | Status |
|-----------|-------------|--------|
| `INetworkPolicy` | Policy interface with apply(NetworkMessage) method | Implemented |
| `RetryPolicy` | Configurable retry strategy (exponential backoff/fixed interval/linear backoff) | Implemented |
| `TimeoutPolicy` | Request timeout policy | Implemented |

#### Interceptor Layer

| Component | Description | Status |
|-----------|-------------|--------|
| `IInterceptor` | Protocol-agnostic interceptor interface (onRequest/onResponse) | Implemented |
| `LoggingInterceptor` | Logs request/response details (API/payload size/duration) | Implemented |
| `AuthInterceptor` | Injects Authorization header into requests | Implemented |

#### Factory Layer

| Component | Description | Status |
|-----------|-------------|--------|
| `NetworkFactory` | Creates INetwork instances by protocol name (http/tcp/ws) | Implemented |

#### Legacy Components

| Component | Description | Status |
|-----------|-------------|--------|
| `HttpClient` | Async/sync HTTP client (timeout/proxy/SSL) | Implemented |
| `HttpRequest` | Request encapsulation | Implemented |
| `HttpResponse` | Response encapsulation | Implemented |
| `HttpApi` | Business API layer (auto serialization/deserialization) | Implemented |
| `Downloader` | Resumable downloader | Implemented |
| `Uploader` | Multipart form/large file chunked upload | Implemented |
| `WebSocket` | WebSocket client (auto-reconnect/heartbeat) | Implemented |
| `TcpClient` | Long connection TCP client | Implemented |
| `CookieJar` | Memory/persistent cookie management | Implemented |
| `Session` | Session management (auto-carry auth token) | Implemented |

### Dependencies
- Core
- Logging

### Thread Safety
- Thread-safe

### Protocol Mapping

| Protocol | payload | metadata | statusCode |
|----------|---------|----------|------------|
| HTTP | body | headers | HTTP status code |
| TCP | raw data | {packetType, sequence} | 0 |
| WebSocket | message content | {messageType: "text"/"binary"} | 0 |

### Usage Example

```cpp
auto network = NetworkFactory::create("http");
network->addInterceptor(std::make_shared<LoggingInterceptor>());
network->addInterceptor(std::make_shared<AuthInterceptor>("token"));
network->setPolicy(std::make_shared<TimeoutPolicy>(30000));

NetworkMessage msg;
msg.api = "/api/users";
msg.payload = "{\"name\":\"test\"}";

auto result = network->send(msg);
```

---

## Module: Storage

### Responsibility
Data persistence with unified Repository interface.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `SqliteDatabase` | Thread-safe SQLite wrapper (WAL mode/encryption reserved) | Implemented |
| `Settings` | User settings (QSettings replacement, unified JSON) | Implemented |
| `ICache<K,V>` | Cache interface (TTL/capacity limit) | Implemented |
| `MemoryCache` | LRU memory cache | Implemented |
| `DiskCache` | Disk cache (files by Key) | Implemented |
| `IRepository<T>` | Repository interface | Implemented |
| `SqliteRepository<T>` | SQLite Repository base implementation | Implemented |
| `MemoryRepository<T>` | In-memory Repository (for testing) | Implemented |
| `ISerializer` | Serialization interface (JSON implementation) | Implemented |
| `JsonSerializer` | JSON serializer | Implemented |

### Dependencies
- Core

### Thread Safety
- Thread-safe

### IRepository Interface

```cpp
template <typename T>
class IRepository {
public:
    virtual Result<T> findById(const QString& id) = 0;
    virtual Result<std::vector<T>> findAll() = 0;
    virtual Result<void> save(const T& entity) = 0;
    virtual Result<void> remove(const QString& id) = 0;
};
```

---

## Module: Async

### Responsibility
Modern async task orchestration wrapping Qt6's QFuture/QPromise.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `ThreadPool` | Global thread pool (wraps QThreadPool) | Implemented |
| `Task` | Async task base class (progress/cancel/result) | Implemented |
| `TaskRunner` | Task runner (submits to thread pool) | Implemented |
| `Dispatcher` | Cross-thread dispatcher (ensures UI thread execution) | Implemented |
| `CancelableTask` | Cancelable task (cooperative cancellation) | Implemented |
| `Future<T>` | Wraps QFuture<T>, supports then()/onSuccess()/onFailure() | Implemented |
| `Promise<T>` | Wraps QPromise<T>, pairs with Future | Implemented |

### Dependencies
- Core
- Logging

### Thread Safety
- Thread-safe

### Usage Example

```cpp
auto future = sc::async([]() -> int {
    return heavyComputation();
}).then([](int result) {
    SC_INFO() << "Result: " << result;
});
```

---

## Module: Event

### Responsibility
Decoupled inter-module communication via event bus.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `IEventBus` | Event bus interface (for Mock and replacement) | Implemented |
| `DefaultEventBus` | Default implementation (sync/async dispatch) | Implemented |
| `IMessageBus` | Message bus interface (point-to-point/broadcast) | Implemented |
| `Subscription` | Subscription handle (for unsubscribe) | Implemented |
| `EventPriority` | Event priority (high priority can abort delivery) | Implemented |
| `QtSignalAdapter` | Converts Qt signals to events | Implemented |

### Dependencies
- Core

### Thread Safety
- Thread-safe

### Key Design
- Not a global singleton: passed via dependency injection for testability
- Explicit lifecycle: created and managed by upper layer (e.g., Application)

---

## Module: UI

### Responsibility
UI infrastructure with unified styling and interaction.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `Style` | Style system (colors/corners/margin/padding/fonts) | Implemented |
| `Theme` | Theme (Dark/Light), contains multiple Styles | Implemented |
| `Icon` | Icon system (FontAwesome/SVG, dynamic color) | Implemented |
| `IconManager` | Icon management singleton | Implemented |
| `Dialog` | Generic dialogs (confirm/prompt/progress/input) | Implemented |
| `Toast` | Lightweight notification (auto-dismiss, configurable position) | Implemented |
| `Animation` | Animation utilities (fade/slide/scale based on QPropertyAnimation) | Implemented |
| `Navigation` | Page navigation stack (push/pop/replace) | Implemented |
| `Page` | Page base class (works with Navigation, has lifecycle) | Implemented |
| `Window` | Main window base class (remembers size/position/state) | Implemented |
| `Loading` | Loading indicator (spinner/progress/skeleton) | Implemented |
| `EmptyWidget` | Empty state display (icon + text + action button) | Implemented |
| `GlassWidget` | Glassmorphism widget base class | Implemented |
| `Button` | Push/icon button with animations | Implemented |
| `Card` | Information card with glassmorphism | Implemented |
| `SideBar` | Side navigation bar | Implemented |
| `Input` | Text input field | Implemented |
| `Switch` | Toggle switch | Implemented |
| `Checkbox` | Checkbox | Implemented |
| `Slider` | Slider control | Implemented |
| `ScrollBar` | Custom scrollbar | Implemented |
| `TabBar` | Tab bar | Implemented |
| `Progress` | Progress indicator | Implemented |
| `Spinner` | Spinner animation | Implemented |
| `ToolTip` | Tooltip | Implemented |
| `Avatar` | User avatar | Implemented |
| `Badge` | Badge indicator | Implemented |
| `Dropdown` | Dropdown menu | Implemented |

### Dependencies
- Core
- Base

### Thread Safety
- GUI Thread only

### Style vs Theme Division
- **Style**: Describes "physical properties" (color values, corner radius, font size, spacing)
- **Theme**: A collection of Styles (Dark Theme, Light Theme)

---

## Module: Auth

### Responsibility
Authentication, token management, and permission control.

### Components

| Component | Description | Status |
|-----------|-------------|--------|
| `AuthManager` | Authentication manager | Implemented |
| `TokenManager` | Token storage and refresh | Implemented |
| `Permission` | Permission checking | Implemented |
| `Token` | Token model | Implemented |
| `User` | User model | Implemented |
| `Permissions` | Permission constants | Implemented |

### Dependencies
- Core
- Network
- Storage
- Utils

### Thread Safety
- Thread-safe

---

## Module: Utils

### Responsibility
Collection of high-frequency utility functions, categorized by subdirectory.

### Structure

```
utils/
├── json/          # JSON read/write helpers
├── file/          # File/path operations
├── string/        # String utilities
├── crypto/        # MD5/SHA/AES
├── image/         # Image resize/crop/format conversion
├── process/       # Process management
├── clipboard/     # Clipboard operations
├── datetime/      # Date/time formatting
├── compress/      # ZIP/GZIP compression
└── xml/           # XML read/write helpers
```

### Dependencies
- Core

### Thread Safety
- Thread-safe

### Namespace
- All utilities in `sc::utils::XXX` sub-namespace

---

## Module: CS (Application Layer)

### Responsibility
CS 架构核心模块，对标 Spring Boot 的 Controller/Service/ViewModel 三层架构。作为 `sc::ui` 的扩展包装层，增加 SpringBoot 风格的抽象。

### Layer
**Application Layer** — 依赖 Foundation 层，不依赖 UI 实现细节。

### Components

| Component | Description | Spring Boot 对标 | Status |
|-----------|-------------|-------------------|--------|
| `CsController` | Signal/Slot 驱动的控制器 | `@RestController` | Implemented |
| `CsRouter` | 页面路由表 + 导航 | `DispatcherServlet` + `@RequestMapping` | Implemented |
| `CsService` | 服务基类 (DI 管理) | `@Service` | Implemented |
| `CsViewModel` | UI 状态管理 | `ModelAndView` | Implemented |
| `CsDataBinding` | Entity ↔ ViewModel 双向绑定 | `DataBinder` + `BeanWrapper` | Implemented |
| `CsFormValidator` | 表单校验 | `Validator` + `@Valid` | Implemented |
| `CsErrorHandler` | 全局错误处理 | `@ControllerAdvice` + `@ExceptionHandler` | Implemented |
| `CsDialogManager` | 对话框管理 | Modal/Alert | Implemented |
| `CsWindowManager` | 窗口生命周期管理 | Session/Window | Implemented |
| `CsNavigation` | 页面导航辅助 | `redirect:` / `forward:` | Implemented |
| `CsModule` | 模块注册与生命周期 | `@Configuration` | Implemented |

### 分层职责

```
View (QWidget) → ViewModel → Controller → Service → Repository
```

| 层 | 职责 | 禁止 |
|----|------|------|
| **View** | 纯 UI 渲染 | 不包含业务逻辑，不直接访问 Service |
| **ViewModel** | UI State 管理 (Q_PROPERTY) | 不直接访问 Service/Repository/数据库 |
| **Controller** | Command/Request 分发，路由注册，输入校验 | 不直接访问 Repository |
| **Service** | 业务逻辑，事务管理，领域编排 | 不依赖 UI（Controller/ViewModel/Widget） |
| **Repository** | 数据访问 CRUD | 不包含业务逻辑 |

### Dependencies
- Core (Foundation)
- Base (Foundation)
- Storage (Foundation)
- DI Container (Foundation)

### Thread Safety
- Controller、Router、ViewModel、DialogManager、WindowManager: GUI Thread only
- Service: Thread-safe

### 核心设计原则

1. **强类型分发优先**: Controller 使用成员函数指针注册路由，编译期类型安全
   ```cpp
   route("list", &UserController::listUsers);  // ✅ 推荐
   route("list", "listUsers");                  // ⚠️ 兼容
   ```

2. **CsModule 不持有所有权**: Service/Controller 所有权由 ApplicationContext 集中管理
   ```cpp
   // ❌ 避免: Module 持有所有对象
   class UserModule : public CsModule {
       UserService* m_userService;
       UserRepository* m_userRepository;
       UserController* m_userController;
   };
   
   // ✅ 推荐: Module 仅负责注册
   class UserModule : public CsModule {
       void onRegister() override {
           registerService<UserService>(repo);
           registerController<UserController>(service);
       }
   };
   ```

3. **CS 与 Web 共享业务 Service**: Service 不感知调用方是 CS 还是 Web

### 与 sc::ui 的关系

`sc::cs` 是 `sc::ui` 的扩展包装层，而非替代：
- `CsController` 继承/组合 `sc::ui::Page`，复用页面生命周期
- `CsRouter` 包装 `sc::ui::Navigation`，增加路由表注册
- `CsViewModel` 继承 `sc::ui::BaseViewModel`，增加 Service 注入
- `CsDialogManager` 包装 `sc::ui::Dialog`/`sc::ui::Toast`

---

## Module: Web (Application Layer, Reserved)

### Responsibility
Web 模块预留，未来通过 Web Adapter 复用 CS 业务层。

### Status
**预留，未实现**。仅包含 `README.md`。

### Layer
**Application Layer** — 未来依赖 CS 模块。

### 设计原则
- 不提前实现 WebController/WebRouter/WebEngineView
- 不复制业务架构
- 通过 Web Adapter 将 HTTP 请求转换为 CsController Signal 调用
- 业务 Service 不感知 CS/Web 调用方

### Dependencies
- 待 QtWebEngine 评估后确定

---

## Module: Examples

### Responsibility
Demo applications to validate module functionality.

### Requirements
- At least 2 demos per major module
- Demonstrate typical usage patterns
- Include both logic-only and UI demos

---

## Module: Tests

### Responsibility
Unit tests and integration tests for all modules.

### Requirements
- Core modules: >90% coverage
- UI modules: >70% coverage
- Integration tests for Network + Storage workflows

---

## Module Dependency Summary

```
Dependency Direction (Allowed):
┌──────────────────────────────────────────────────────────────┐
│                        UI Layer                              │
│  UI                                                          │
├──────────────────────────────────────────────────────────────┤
│                      Business Layer                          │
│  Auth                                                        │
├──────────────────────────────────────────────────────────────┤
│                      Infrastructure Layer                    │
│  Network ──→ Logging ──→ Core                                │
│  Storage ──→ Core                                            │
│  Event ──→ Core                                              │
│  Configuration ──→ Utils ──→ Core                            │
│  Async ──→ Logging ──→ Core                                  │
│  Logging ──→ Core                                            │
│  Utils ──→ Core                                              │
├──────────────────────────────────────────────────────────────┤
│                      Foundation Layer                         │
│  Core (Qt Core only)                                         │
│  Base ──→ Core                                               │
└──────────────────────────────────────────────────────────────┘
```

## Module Review Checklist

For new modules, verify:

- ☑ Has interface definitions
- ☑ Has unit tests
- ☑ Has example usage
- ☑ Has Doxygen documentation
- ☑ Has thread safety consideration
- ☑ Has logging integration
- ☑ Uses Result/Error for error handling
- ☑ Follows dependency rules
- ☑ No circular dependencies
- ☑ Follows naming conventions