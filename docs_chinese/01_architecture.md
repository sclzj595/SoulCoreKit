# SoulCoreKit 架构规范

## 概述

SoulCoreKit 遵循分层、模块化架构，具有严格的依赖规则。每个模块都有明确的职责和定义良好的接口。

## 分层架构

```
┌─────────────────────────────────────────────────────────────┐
│                        UI 层                                │
│  Widgets, Themes, Animations, Dialogs, Navigation          │
├─────────────────────────────────────────────────────────────┤
│                      业务层                                 │
│  Auth, Service, Repository, ViewModel                      │
├─────────────────────────────────────────────────────────────┤
│                      基础设施层                             │
│  Network, Storage, Event, Configuration, Logging           │
├─────────────────────────────────────────────────────────────┤
│                      基础层                                 │
│  Core (Result, Error, Interface), Base (BaseObject, etc.)  │
└─────────────────────────────────────────────────────────────┘
```

## 模块结构

```
SoulCoreKit/
├── Core              # 基础（仅依赖 Qt Core）
├── Base              # 基类（扩展 Core）
├── Logging           # 日志系统
├── Configuration     # 配置管理
├── Network           # 网络通信
├── Storage           # 数据持久化
├── Async             # 异步任务
├── Event             # 事件总线
├── UI                # UI 组件
├── Auth              # 认证
├── Utils             # 工具函数
├── Examples          # 演示应用
├── Tests             # 单元测试
└── Docs              # 文档
```

## 模块依赖图

### 允许的依赖

```
UI ────────→ Core
 │            ↑
 └──────→ Base ─┘

Auth ───────→ Core
  │            ↑
  ├──→ Network ─┤
  ├──→ Storage ─┤
  └──→ Utils ────┘

Network ────→ Core
  │            ↑
  └──→ Logging ─┘

Storage ─────→ Core

Configuration → Core
  │            ↑
  └──→ Utils ──┘

Event ───────→ Core

Async ───────→ Core
  │            ↑
  └──→ Logging ─┘

Logging ─────→ Core

Utils ───────→ Core

Base ────────→ Core
```

### 禁止的依赖

- **Core** 不得依赖任何其他模块
- **基础设施模块**（Network、Storage、Event 等）不得依赖 UI
- **禁止任何模块间的循环依赖**

## 模块职责矩阵

| 模块 | 职责 | 依赖 | 线程安全 |
|------|------|------|----------|
| **Core** | 基础类型（Result、Error、Interface、Singleton） | Qt Core | 是 |
| **Base** | 基类（BaseObject、BaseManager 等） | Core、UI | 是 |
| **Logging** | 日志系统，支持 sink 架构 | Core | 是 |
| **Configuration** | 配置管理，热重载 | Core、Utils | 是 |
| **Network** | HTTP、WebSocket、TCP、下载/上传 | Core、Logging | 是 |
| **Storage** | SQLite、缓存、设置、序列化 | Core | 是 |
| **Async** | ThreadPool、Task、Dispatcher | Core、Logging | 是 |
| **Event** | 事件总线、订阅管理 | Core | 是 |
| **UI** | Widget、主题、动画、对话框 | Core、Base | 仅 GUI 线程 |
| **Auth** | 认证、Token、权限 | Core、Network、Storage | 是 |
| **Utils** | 辅助工具（json、文件、字符串等） | Core | 是 |

## 组件交互

### 应用生命周期

```
Application::start()
    │
    ├── Core::initialize()
    ├── Logging::initialize()
    ├── Configuration::load()
    ├── EventBus::create()
    ├── Storage::initialize()
    ├── Network::initialize()
    ├── UI::initialize()
    │
    └── Application::run()
            │
            └── Application::shutdown()
                    ├── UI::shutdown()
                    ├── Network::shutdown()
                    ├── Storage::shutdown()
                    ├── EventBus::destroy()
                    ├── Configuration::save()
                    ├── Logging::shutdown()
                    └── Core::shutdown()
```

### 依赖注入模式

服务应通过构造函数注入，而非全局访问：

```cpp
// 推荐
class UserService {
public:
    UserService(INetworkClient* network, IStorage* storage)
        : m_network(network), m_storage(storage) {}
private:
    INetworkClient* m_network;
    IStorage* m_storage;
};

// 禁止
class UserService {
public:
    UserService() {
        m_network = NetworkClient::instance();  // 全局访问
    }
};
```

## 关键设计模式

### 1. 基于接口的设计

所有公共服务必须暴露接口：

```cpp
class INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual Result<HttpResponse> get(const QString& url) = 0;
    virtual Result<HttpResponse> post(const QString& url, const QByteArray& body) = 0;
};
```

### 2. 受控生命周期的单例

单例必须实现 `init()` 和 `shutdown()`：

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

### 3. 工厂模式

使用工厂按名称创建对象：

```cpp
class IFactory {
public:
    virtual QObject* create(const QString& type) = 0;
};
```

### 4. 观察者模式

事件总线实现观察者模式用于解耦通信：

```cpp
class IEventBus {
public:
    virtual Subscription subscribe(const QString& eventType, Callback callback) = 0;
    virtual void publish(const QString& eventType, const QVariant& data) = 0;
};
```

## 数据流

### 请求-响应流程

```
UI 层                             基础设施层
   │                                     │
   │── 用户操作 ──→ ViewModel            │
   │         │                           │
   │         └── Service::execute()      │
   │                     │               │
   │                     ↓               │
   │              Network::request() ────┼──→ HTTP 请求
   │                     │               │
   │                     ↓               │
   │              Network::response() ←──┼──← HTTP 响应
   │                     │               │
   │                     ↓               │
   │              Service::handleResult()│
   │                     │               │
   │                     ↓               │
   │         ViewModel::updateState()    │
   │                     │               │
   │                     ↓               │
   │── UI::update() ←───┘               │
```

### 事件驱动流程

```
生产者                          EventBus                          消费者
   │                                 │                                 │
   │── publish(event) ───────────────→│                                 │
   │                                 │── dispatch(event) ──────────────→│
   │                                 │                                 │── handle(event)
```

## 架构约束

1. **基础设施中无 UI**：Network、Storage、Event 模块不得包含任何 UI 代码
2. **UI 中无业务逻辑**：UI 组件只能处理展示，不能包含业务逻辑
3. **接口隔离**：接口应小而专注
4. **依赖倒置**：依赖抽象，而非具体实现
5. **单一职责**：每个类应该只有一个修改的理由

## 避免的反模式

- **上帝对象**：做太多事情的类（例如，既处理服务又处理 UI 的类）
- **全局状态**：避免滥用单例；优先使用依赖注入
- **循环依赖**：使用接口打破循环
- **深层继承**：优先组合而非继承
- **魔法字符串**：使用枚举或常量替代原始字符串