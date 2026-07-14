# SoulCoreKit 基础框架说明

## 一、框架概述

SoulCoreKit 是一个基于 Qt6 的现代化 C++ 应用程序开发框架，专为构建高性能、可扩展的跨平台应用而设计。框架采用模块化架构，遵循 Architecture First 原则，提供了从核心基础设施到 UI 组件的完整解决方案。

### 核心特性

| 特性 | 说明 |
|------|------|
| **模块化架构** | 12 个独立模块，按需组合，低耦合高内聚 |
| **协议无关网络层** | 统一 HTTP/TCP/WebSocket 接口，支持策略模式和拦截器链 |
| **事件驱动架构** | 基于发布-订阅模式的事件总线，支持 Qt 信号桥接 |
| **异步任务框架** | 基于线程池的异步执行，支持协程风格编程 |
| **统一错误处理** | `Result<T>` 模式，类型安全的错误传播 |
| **可扩展存储层** | 支持内存、文件、SQLite 多种存储后端 |
| **声明式 UI** | 现代化 UI 组件库，支持主题切换和玻璃态效果 |

---

## 二、架构设计

### 2.1 整体架构图

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         应用层 (Application)                           │
│                         使用框架提供的 API                              │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          业务服务层 (Service Layer)                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │ soul_auth│  │ soul_base│  │ soul_ui  │  │ soul_app │  │ soul_... │ │
│  │ 认证服务 │  │ 基础组件 │  │ 用户界面 │  │ 应用管理 │  │ 其他模块 │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘ │
│       │             │             │             │             │        │
└───────┼─────────────┼─────────────┼─────────────┼─────────────┼────────┘
        │             │             │             │             │
        ▼             ▼             ▼             ▼             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          基础设施层 (Infrastructure)                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │soul_netw │  │soul_async│  │soul_event│  │soul_stor │  │soul_log  │ │
│  │  网络模块 │  │ 异步执行 │  │ 事件总线 │  │  存储模块 │  │  日志模块 │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘ │
│       │             │             │             │             │        │
└───────┼─────────────┼─────────────┼─────────────┼─────────────┼────────┘
        │             │             │             │             │
        ▼             ▼             ▼             ▼             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          核心层 (Core)                                  │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │ soul_core│  │soul_util │  │soul_conf │  │   Qt6    │  │  C++17   │ │
│  │ 核心基础 │  │ 工具函数 │  │ 配置管理 │  │   框架   │  │  标准库  │ │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

### 2.2 模块依赖关系

```
                    soul_core (核心)
                         │
         ┌───────────────┼───────────────┐
         ▼               ▼               ▼
    soul_utils      soul_logging      soul_async
         │               │               │
         └───────┬───────┴───────┬───────┘
                 ▼               ▼
          soul_configuration  soul_event
                 │               │
                 └───────┬───────┘
                         ▼
              soul_network (网络)
                         │
                         ▼
              soul_storage (存储)
                         │
                         ▼
              soul_auth (认证)
                         │
                         ▼
              soul_ui (界面)
                         │
                         ▼
              soul_base (业务基础)
```

### 2.3 模块职责表

| 模块 | 职责 | 核心类 | 文件数 | 行数(约) |
|------|------|--------|--------|----------|
| **soul_core** | 核心基础设施 | `Result<T>`, `Error`, `IInterface`, `Singleton`, `Factory<T>` | 11 | ~200 |
| **soul_utils** | 工具函数库 | JSON/文件/字符串/加密/图像处理工具 | 10 | ~500 |
| **soul_logging** | 日志系统 | `Logger`, `ISink`, `LogFormatter`, 多输出目标 | 12 | ~300 |
| **soul_configuration** | 配置管理 | `IConfiguration`, `JsonConfiguration`, `IniConfiguration` | 5 | ~200 |
| **soul_async** | 异步执行 | `ThreadPool`, `TaskRunner`, `Future`, `Promise` | 8 | ~300 |
| **soul_event** | 事件总线 | `EventBus`, `Subscription`, `QtSignalAdapter` | 6 | ~200 |
| **soul_network** | 网络通信 | `INetwork`, `NetworkFactory`, 策略/拦截器/适配器 | 22 | ~800 |
| **soul_storage** | 数据存储 | `IStorage`, `SqliteDatabase`, `Cache`, `Repository` | 9 | ~300 |
| **soul_auth** | 认证管理 | `AuthManager`, `TokenManager`, `Permission` | 6 | ~200 |
| **soul_ui** | 用户界面 | 30+ UI 组件, Theme, Style, Animation | 40 | ~1200 |
| **soul_base** | 业务基础 | `BaseService`, `BaseRepository`, `BaseWidget` | 7 | ~200 |
| **SoulCoreKit** | 聚合目标 | 所有模块的统一入口 | 1 | - |

---

## 三、核心模块详解

### 3.1 soul_core - 核心基础设施

**职责**：提供框架的核心基础设施，包括统一接口、错误处理、单例模式、工厂模式等。

**核心类**：

| 类名 | 职责 | 关键方法 |
|------|------|----------|
| `Result<T>` | 类型安全的返回值 | `isOk()`, `isErr()`, `unwrap()`, `unwrapErr()` |
| `Error` | 错误信息封装 | `code()`, `message()`, `toString()` |
| `IInterface` | 接口基类 | `interfaceName()` |
| `Singleton<T>` | 单例模板 | `instance()` |
| `Factory<T>` | 工厂模板 | `registerCreator()`, `create()` |
| `UUID` | UUID 生成 | `generate()`, `toString()` |
| `Version` | 版本管理 | `major()`, `minor()`, `patch()` |

**使用示例**：

```cpp
#include "soul/core/result.h"
#include "soul/core/singleton.h"

// 错误处理
Result<int> divide(int a, int b) {
    if (b == 0) {
        return Error(1, "Division by zero");
    }
    return a / b;
}

auto result = divide(10, 2);
if (result.isOk()) {
    qDebug() << "Result:" << result.unwrap();
}

// 单例模式
class MyManager : public Singleton<MyManager> {
public:
    void doSomething() {}
};

MyManager::instance().doSomething();
```

### 3.2 soul_network - 网络通信模块

**职责**：提供协议无关的统一网络接口，支持 HTTP/TCP/WebSocket，内置策略模式和拦截器链。

**架构设计**：

```
┌─────────────┐    ┌───────────────┐    ┌─────────────────────┐
│   INetwork  │    │ NetworkBase   │    │ NetworkAdapterBase  │
│  (纯虚接口) │    │  (信号基类)   │    │   (适配器基类)      │
└──────┬──────┘    └───────┬───────┘    └──────────┬──────────┘
       │                   │                       │
       └────────┬──────────┴───────────┬───────────┘
                ▼                      ▼
    ┌────────────────────┐   ┌─────────────────────┐
    │   NetworkFactory   │   │   NetworkMessage    │
    │   (工厂模式)       │   │   (消息模型)        │
    └────────┬───────────┘   └─────────────────────┘
             │
    ┌────────┼────────┐
    ▼        ▼        ▼
┌────────┐┌────────┐┌────────┐
│ HTTP   ││ TCP    ││ WebSocket │
│Adapter ││Adapter ││ Adapter  │
└────────┘└────────┘└────────┘
```

**核心组件**：

| 组件 | 职责 |
|------|------|
| `INetwork` | 统一网络接口 |
| `NetworkBase` | Qt 信号定义 |
| `NetworkAdapterBase` | 适配器基类，处理策略和拦截器 |
| `NetworkMessage` | 协议中立的消息模型 |
| `NetworkFactory` | 按协议创建网络实例 |
| `INetworkPolicy` | 策略接口（重试、超时） |
| `IInterceptor` | 拦截器接口（请求/响应） |

**使用示例**：

```cpp
#include "soul/network/soul_network.h"

// 创建网络实例
auto http = sc::network::NetworkFactory::instance().create(QUrl("http://api.example.com"));
auto tcp = sc::network::NetworkFactory::instance().create(QUrl("tcp://localhost:8080"));
auto ws = sc::network::NetworkFactory::instance().create(QUrl("ws://localhost:8080"));

// 配置策略和拦截器
http->addInterceptor(std::make_shared<sc::network::LoggingInterceptor>());
http->setPolicy(std::make_shared<sc::network::RetryPolicy>(3));

// 发送消息
sc::network::NetworkMessage msg;
msg.api = "/api/users";
msg.payload = "{\"name\":\"test\"}";

auto result = http->send(msg);
if (result.isOk()) {
    qDebug() << result.unwrap().payload;
}

// 异步发送
http->sendAsync(msg, [](const sc::Result<sc::network::NetworkMessage>& result) {
    // 处理响应
});

// 监听事件
connect(http.get(), &sc::network::NetworkBase::connected, []() {
    qDebug() << "Connected!";
});
```

### 3.3 soul_event - 事件总线

**职责**：提供发布-订阅模式的事件通信机制，支持 Qt 信号桥接。

**使用示例**：

```cpp
#include "soul/event/event_bus.h"

// 定义事件
class UserLoggedInEvent : public sc::IEvent {
public:
    std::string userId;
    std::string interfaceName() const override { return "UserLoggedInEvent"; }
};

// 订阅事件
auto subscription = sc::EventBus::instance().subscribe<UserLoggedInEvent>([](const UserLoggedInEvent& event) {
    qDebug() << "User logged in:" << event.userId.c_str();
});

// 发布事件
sc::EventBus::instance().publish(UserLoggedInEvent{"user123"});
```

### 3.4 soul_async - 异步任务框架

**职责**：提供基于线程池的异步执行框架，支持任务调度和取消。

**使用示例**：

```cpp
#include "soul/async/thread_pool.h"
#include "soul/async/future.h"

// 提交异步任务
auto future = sc::ThreadPool::instance().submit([]() {
    // 耗时操作
    return "result";
});

// 获取结果
auto result = future.get();
```

### 3.5 soul_storage - 存储模块

**职责**：提供可扩展的数据存储接口，支持内存、文件、SQLite 后端。

**使用示例**：

```cpp
#include "soul/storage/sqlite_database.h"
#include "soul/storage/repository.h"

auto db = std::make_shared<sc::SqliteDatabase>("data.db");
auto repo = sc::Repository<User>(db);

User user{"user1", "John"};
repo.save(user);

auto found = repo.findById("user1");
if (found.isOk()) {
    qDebug() << found.unwrap().name;
}
```

### 3.6 soul_ui - UI 组件库

**职责**：提供现代化的 Qt Widgets UI 组件，支持主题切换和玻璃态效果。

**核心组件**：

| 组件 | 说明 |
|------|------|
| `Window` | 主窗口框架 |
| `Page` | 页面容器 |
| `Sidebar` | 侧边栏导航 |
| `Card` | 卡片组件 |
| `Button` | 按钮组件 |
| `Input` | 输入框 |
| `Dialog` | 对话框 |
| `Toast` | 轻量级提示 |
| `Theme` | 主题管理 |
| `Animation` | 动画系统 |

---

## 四、设计原则

### 4.1 Architecture First

框架遵循 Architecture First 原则，核心设计决策在代码编写前完成。所有模块都有清晰的职责边界和接口定义。

### 4.2 接口与实现分离

所有核心功能通过接口定义，实现类继承接口。这种设计使得：
- 易于单元测试（可 Mock）
- 易于扩展（可替换实现）
- 编译时类型安全

### 4.3 统一错误处理

使用 `Result<T>` 模式统一处理错误，避免异常滥用，提供类型安全的错误传播机制。

### 4.4 依赖注入友好

框架设计支持依赖注入，所有组件通过接口引用，便于测试和替换。

### 4.5 线程安全

关键组件（EventBus、ThreadPool、Logger）设计为线程安全，可在多线程环境中安全使用。

---

## 五、构建系统

### 5.1 CMake 配置

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Sql Widgets)

add_subdirectory(SoulCoreKit)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE SoulCoreKit)
```

### 5.2 模块链接关系

```
soul_core ──────────────────────┐
    │                           │
soul_utils ───────┐             │
    │             │             │
soul_logging ─────┼─ soul_async ─┼─ soul_event ────┐
    │             │             │                  │
soul_configuration ──────────────┘                  │
    │                                              │
soul_network ───────────────────────────────────────┘
    │
soul_storage ─── soul_auth ─── soul_ui ─── soul_base
```

---

## 六、项目结构

```
SoulCoreKit/
├── include/                    # 头文件目录
│   └── soul/
│       ├── core/               # 核心基础设施
│       ├── network/            # 网络通信
│       ├── event/              # 事件总线
│       ├── async/              # 异步执行
│       ├── storage/            # 数据存储
│       ├── logging/            # 日志系统
│       ├── configuration/      # 配置管理
│       ├── utils/              # 工具函数
│       ├── auth/               # 认证管理
│       ├── ui/                 # 用户界面
│       └── base/               # 业务基础
├── src/                        # 源文件目录
│   └── soul/                   # 与 include 对应
├── tests/                      # 单元测试
├── examples/                   # 示例程序
├── docs/                       # 英文文档
├── docs_chinese/               # 中文文档
├── CMakeLists.txt              # 主构建文件
└── Doxyfile                    # Doxygen 配置
```

---

## 七、快速开始

### 7.1 创建项目

```cpp
#include "soul/core/application.h"
#include "soul/logging/logger.h"
#include "soul/network/soul_network.h"

int main(int argc, char* argv[]) {
    sc::Application app(argc, argv);
    
    // 初始化日志
    sc::Logger::instance().addSink(std::make_shared<sc::ConsoleSink>());
    
    // 创建网络实例
    auto network = sc::network::NetworkFactory::instance().create(QUrl("http://api.example.com"));
    
    // 发送请求
    sc::network::NetworkMessage msg;
    msg.api = "/api/hello";
    auto result = network->send(msg);
    
    SC_LOG_INFO("Response: {}", result.isOk() ? "success" : "failed");
    
    return app.exec();
}
```

### 7.2 CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Widgets)

add_subdirectory(SoulCoreKit)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE SoulCoreKit)
```

---

## 八、技术栈

| 技术 | 版本 | 用途 |
|------|------|------|
| C++ | 17 | 核心开发语言 |
| Qt | 6.5.3 | GUI 框架、网络、数据库 |
| CMake | 3.16+ | 构建系统 |
| SQLite | 内置 | 嵌入式数据库 |
| Doxygen | - | 文档生成 |

---

## 九、性能指标

| 指标 | 目标值 |
|------|--------|
| 事件总线吞吐量 | > 100,000 事件/秒 |
| 网络请求延迟 | < 1ms (本地) |
| 线程池任务调度 | < 0.1ms |
| SQLite 查询 | < 1ms (简单查询) |

---

## 十、总结

SoulCoreKit 是一个功能完整、架构清晰的 Qt6 C++ 应用框架，具备以下优势：

1. **模块化设计**：12 个独立模块，按需组合
2. **协议无关网络**：统一 HTTP/TCP/WebSocket 接口
3. **事件驱动架构**：高效的发布-订阅机制
4. **异步执行**：基于线程池的任务调度
5. **统一错误处理**：`Result<T>` 类型安全模式
6. **丰富的 UI 组件**：30+ 现代化组件
7. **向后兼容**：支持渐进式迁移

框架适用于构建各类 Qt 应用，从桌面工具到企业级应用均可覆盖。

---

**项目名称**: SoulCoreKit  
**版本**: 1.0.0  
**许可证**: MIT License  
**开发状态**: 活跃开发中  
**最后更新**: 2026-07-13