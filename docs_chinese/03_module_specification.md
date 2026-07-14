# SoulCoreKit 模块规范

## 概述

本文档指定了 SoulCoreKit 中每个模块的职责、接口和约束。

---

## 模块：Core

### 职责

基础类型和工具，除 Qt Core 外无第三方依赖。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `Result<T>` | Rust 风格的结果类型：Ok(T) / Err(Error)，支持链式调用 | 已实现 |
| `Error` | 统一错误码 + 消息 + 嵌套错误支持 | 已实现 |
| `IInterface` | 用于 RTTI 的纯虚接口基类 | 已实现 |
| `IFactory<T>` | 用于按名称创建对象的抽象工厂接口 | 已实现 |
| `Singleton<T>` | 线程安全的单例模板，具有受控生命周期 | 已实现 |
| `Application` | 包装 QApplication，提供生命周期管理 | 已实现 |
| `Platform` | 运行时操作系统/架构/环境检测 | 已实现 |
| `Environment` | 环境变量、命令行解析、工作目录 | 已实现 |
| `Version` | SemVer 2.0 解析和比较 | 已实现 |
| `Uuid` | UUID 生成/解析（扩展 QUuid） | 已实现 |
| `Time` | 时间戳、定时器、时区转换、格式化 | 已实现 |

### 依赖
- 仅 Qt Core

### 线程安全
- 所有组件必须是线程安全的

---

## 模块：Base

### 职责

所有业务模块的基类。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `BaseObject` | 继承 QObject，提供调试名称、对象树管理 | 已实现 |
| `BaseManager` | 管理器基类（单例或全局访问），带 init/shutdown | 已实现 |
| `BaseService` | 服务基类（业务逻辑容器，支持依赖注入） | 已实现 |
| `BaseRepository` | Repository 基类（配合 Storage 使用） | 已实现 |
| `BaseWidget` | UI 控件基类（集成 Style 系统） | 已实现 |
| `BaseDialog` | 对话框基类（统一布局、按钮、事件处理） | 已实现 |
| `BaseViewModel` | MVVM ViewModel 基类（支持数据绑定） | 已实现 |

### 依赖
- Core

### 线程安全
- 线程安全

---

## 模块：Logging

### 职责

工业级日志系统，支持 sink 架构。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `Logger` | 日志门面：debug()/info()/warn()/error()/fatal() | 已实现 |
| `LogLevel` | 枚举：Trace/Debug/Info/Warn/Error/Fatal | 已实现 |
| `ISink` | 日志输出接口，用于自定义扩展 | 已实现 |
| `ConsoleSink` | 控制台输出，支持颜色 | 已实现 |
| `FileSink` | 文件输出，支持基于大小的滚动 | 已实现 |
| `DailyFileSink` | 每日文件滚动 | 已实现 |
| `CallbackSink` | 回调输出，用于 UI 显示或远程上报 | 已实现 |
| `CompositeSink` | 组合多个 sink | 已实现 |
| `LogFormatter` | 日志格式化：时间/级别/文件/行号/消息 | 已实现 |

### 依赖
- Core

### 线程安全
- 线程安全

### 使用示例

```cpp
SC_INFO("Network") << "Connected to server " << url;
SC_WARN("Storage") << "Cache miss for key " << key;
```

---

## 模块：Configuration

### 职责

分层配置管理，支持多文件和热重载。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `Config` | 配置入口：加载/保存/监听变更 | 已实现 |
| `IConfiguration` | 配置源接口 | 已实现 |
| `JsonConfiguration` | JSON 文件配置源 | 已实现 |
| `IniConfiguration` | INI 文件配置源 | 已实现 |
| `ConfigSchema` | 配置模式定义（类型验证、默认值） | 已实现 |

### 依赖
- Core
- Utils

### 线程安全
- 线程安全

### 推荐配置目录

```
config/
  app.json          # 应用基础配置
  network.json      # 网络（超时/重试/代理）
  ui.json           # UI（字体/大小/默认主题）
  theme.json        # 主题（颜色/圆角/间距）
  logging.json      # 日志（级别/sink 配置）
  [env]/            # 环境覆盖（dev/prod）
```

---

## 模块：Network

### 职责

完整的网络通信栈，从底层连接到业务 API。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `HttpClient` | 异步/同步 HTTP 客户端（超时/代理/SSL） | 已实现 |
| `HttpRequest` | 请求封装 | 已实现 |
| `HttpResponse` | 响应封装 | 已实现 |
| `HttpApi` | 业务 API 层（自动序列化/反序列化） | 已实现 |
| `Downloader` | 断点续传下载器 | 已实现 |
| `Uploader` | 多部分表单/大文件分片上传 | 已实现 |
| `WebSocket` | WebSocket 客户端（自动重连/心跳） | 已实现 |
| `TcpClient` | 长连接 TCP 客户端 | 已实现 |
| `RetryPolicy` | 可配置的重试策略（指数退避/固定间隔） | 已实现 |
| `IInterceptor` | 请求/响应拦截器接口 | 已实现 |
| `CookieJar` | 内存/持久化 Cookie 管理 | 已实现 |
| `Session` | 会话管理（自动携带认证令牌） | 已实现 |
| `NetworkError` | 网络错误分类和映射 | 已实现 |

### 依赖
- Core
- Logging

### 线程安全
- 线程安全

---

## 模块：Storage

### 职责

数据持久化，统一 Repository 接口。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `SqliteDatabase` | 线程安全的 SQLite 封装（WAL 模式/加密预留） | 已实现 |
| `Settings` | 用户设置（QSettings 替代品，统一 JSON） | 已实现 |
| `ICache<K,V>` | 缓存接口（TTL/容量限制） | 已实现 |
| `MemoryCache` | LRU 内存缓存 | 已实现 |
| `DiskCache` | 磁盘缓存（按 Key 存储文件） | 已实现 |
| `IRepository<T>` | Repository 接口 | 已实现 |
| `SqliteRepository<T>` | SQLite Repository 基础实现 | 已实现 |
| `MemoryRepository<T>` | 内存 Repository（用于测试） | 已实现 |
| `ISerializer` | 序列化接口（JSON 实现） | 已实现 |
| `JsonSerializer` | JSON 序列化器 | 已实现 |

### 依赖
- Core

### 线程安全
- 线程安全

### IRepository 接口

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

## 模块：Async

### 职责

现代异步任务编排，包装 Qt6 的 QFuture/QPromise。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `ThreadPool` | 全局线程池（包装 QThreadPool） | 已实现 |
| `Task` | 异步任务基类（进度/取消/结果） | 已实现 |
| `TaskRunner` | 任务运行器（提交到线程池） | 已实现 |
| `Dispatcher` | 跨线程调度器（确保在 UI 线程执行） | 已实现 |
| `CancelableTask` | 可取消任务（协作式取消） | 已实现 |
| `Future<T>` | 包装 QFuture<T>，支持 then()/onSuccess()/onFailure() | 已实现 |
| `Promise<T>` | 包装 QPromise<T>，与 Future 配对 | 已实现 |

### 依赖
- Core
- Logging

### 线程安全
- 线程安全

### 使用示例

```cpp
auto future = sc::async([]() -> int {
    return heavyComputation();
}).then([](int result) {
    SC_INFO() << "Result: " << result;
});
```

---

## 模块：Event

### 职责

通过事件总线实现模块间解耦通信。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `IEventBus` | 事件总线接口（用于 Mock 和替换） | 已实现 |
| `DefaultEventBus` | 默认实现（同步/异步分发） | 已实现 |
| `IMessageBus` | 消息总线接口（点对点/广播） | 已实现 |
| `Subscription` | 订阅句柄（用于取消订阅） | 已实现 |
| `EventPriority` | 事件优先级（高优先级可中止投递） | 已实现 |
| `QtSignalAdapter` | 将 Qt 信号转换为事件 | 已实现 |

### 依赖
- Core

### 线程安全
- 线程安全

### 关键设计
- 不是全局单例：通过依赖注入传递以提高可测试性
- 显式生命周期：由上层（如 Application）创建和管理

---

## 模块：UI

### 职责

UI 基础设施，统一样式和交互。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `Style` | 样式系统（颜色/圆角/边距/内边距/字体） | 已实现 |
| `Theme` | 主题（暗色/亮色），包含多个 Style | 已实现 |
| `Icon` | 图标系统（FontAwesome/SVG，动态颜色） | 已实现 |
| `IconManager` | 图标管理单例 | 已实现 |
| `Dialog` | 通用对话框（确认/提示/进度/输入） | 已实现 |
| `Toast` | 轻量级通知（自动消失，可配置位置） | 已实现 |
| `Animation` | 动画工具（基于 QPropertyAnimation 的淡入/滑动/缩放） | 已实现 |
| `Navigation` | 页面导航栈（push/pop/replace） | 已实现 |
| `Page` | 页面基类（与 Navigation 配合，有生命周期） | 已实现 |
| `Window` | 主窗口基类（记住大小/位置/状态） | 已实现 |
| `Loading` | 加载指示器（旋转器/进度/骨架屏） | 已实现 |
| `EmptyWidget` | 空状态展示（图标 + 文字 + 操作按钮） | 已实现 |
| `GlassWidget` | 毛玻璃控件基类 | 已实现 |
| `Button` | 按钮/图标按钮，带动画 | 已实现 |
| `Card` | 信息卡片，带毛玻璃效果 | 已实现 |
| `SideBar` | 侧边导航栏 | 已实现 |
| `Input` | 文本输入框 | 已实现 |
| `Switch` | 开关切换 | 已实现 |
| `Checkbox` | 复选框 | 已实现 |
| `Slider` | 滑块控件 | 已实现 |
| `ScrollBar` | 自定义滚动条 | 已实现 |
| `TabBar` | 标签栏 | 已实现 |
| `Progress` | 进度指示器 | 已实现 |
| `Spinner` | 旋转动画 | 已实现 |
| `ToolTip` | 工具提示 | 已实现 |
| `Avatar` | 用户头像 | 已实现 |
| `Badge` | 徽章指示器 | 已实现 |
| `Dropdown` | 下拉菜单 | 已实现 |

### 依赖
- Core
- Base

### 线程安全
- 仅 GUI 线程

### Style vs Theme 划分
- **Style**：描述"物理属性"（颜色值、圆角、字体大小、间距）
- **Theme**：一组 Style 的集合（暗色主题、亮色主题）

---

## 模块：Auth

### 职责

认证、令牌管理和权限控制。

### 组件

| 组件 | 描述 | 状态 |
|------|------|------|
| `AuthManager` | 认证管理器 | 已实现 |
| `TokenManager` | 令牌存储和刷新 | 已实现 |
| `Permission` | 权限检查 | 已实现 |
| `Token` | 令牌模型 | 已实现 |
| `User` | 用户模型 | 已实现 |
| `Permissions` | 权限常量 | 已实现 |

### 依赖
- Core
- Network
- Storage
- Utils

### 线程安全
- 线程安全

---

## 模块：Utils

### 职责

高频工具函数集合，按子目录分类。

### 结构

```
utils/
├── json/          # JSON 读写辅助
├── file/          # 文件/路径操作
├── string/        # 字符串工具
├── crypto/        # MD5/SHA/AES
├── image/         # 图片缩放/裁剪/格式转换
├── process/       # 进程管理
├── clipboard/     # 剪贴板操作
├── datetime/      # 日期/时间格式化
├── compress/      # ZIP/GZIP 压缩
└── xml/           # XML 读写辅助
```

### 依赖
- Core

### 线程安全
- 线程安全

### 命名空间
- 所有工具位于 `sc::utils::XXX` 子命名空间

---

## 模块：Examples

### 职责

演示应用，用于验证模块功能。

### 要求
- 每个主要模块至少 2 个演示
- 演示典型使用模式
- 包含纯逻辑和 UI 演示

---

## 模块：Tests

### 职责

所有模块的单元测试和集成测试。

### 要求
- 核心模块：>90% 覆盖率
- UI 模块：>70% 覆盖率
- Network + Storage 工作流集成测试

---

## 模块依赖总结

```
依赖方向（允许）：
┌──────────────────────────────────────────────────────────────┐
│                        UI 层                                 │
│  UI                                                          │
├──────────────────────────────────────────────────────────────┤
│                      业务层                                  │
│  Auth                                                        │
├──────────────────────────────────────────────────────────────┤
│                      基础设施层                              │
│  Network ──→ Logging ──→ Core                                │
│  Storage ──→ Core                                            │
│  Event ──→ Core                                              │
│  Configuration ──→ Utils ──→ Core                            │
│  Async ──→ Logging ──→ Core                                  │
│  Logging ──→ Core                                            │
│  Utils ──→ Core                                              │
├──────────────────────────────────────────────────────────────┤
│                      基础层                                  │
│  Core（仅 Qt Core）                                           │
│  Base ──→ Core                                               │
└──────────────────────────────────────────────────────────────┘
```

## 模块审查清单

对于新模块，验证：

- ☑ 有接口定义
- ☑ 有单元测试
- ☑ 有示例用法
- ☑ 有 Doxygen 文档
- ☑ 有线程安全考虑
- ☑ 有日志集成
- ☑ 使用 Result/Error 进行错误处理
- ☑ 遵循依赖规则
- ☑ 无循环依赖
- ☑ 遵循命名规范