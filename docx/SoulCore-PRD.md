# SoulCoreKit 产品需求文档（PRD）

| 属性 | 值 |
|------|-----|
| 版本 | V1.0 |
| 状态 | 草稿 |
| 目标平台 | Qt 6.5+ / C++17 |
| 定位 | 个人桌面应用基础设施平台库（Application Infrastructure Library） |

## 一、核心理念与定位

### 1.1 定位

SoulCoreKit 是一套轻量级、模块化、演进式的 C++/Qt 基础设施库，服务于作者所有桌面应用项目（IDE、Music、Radio、Notebook、IM、EatDecider 等）。它不是 Qt 的替代品，而是对 Qt 生态的补充与规范，解决跨项目代码复用问题。

### 1.2 核心准则（"两次原则"）

任何一个模块，只有当它在至少两个项目中被重复使用时，才纳入 SoulCoreKit。

这条原则是防止过度设计的"防火墙"：

- 不预测需求，只响应已验证的需求。
- 保持库精简、稳定、易维护。

V1 目标代码规模：5 万～8 万行。

### 1.3 与 Qt 的分工

| 功能 | Qt 原生 | SoulCoreKit 职责 |
|------|---------|------------------|
| 对象模型 | QObject | 提供统一的基类体系（BaseObject → 业务对象） |
| JSON | QJsonDocument | 提供更友好的取值/赋值语法糖 |
| 线程 | QThread / QThreadPool | 提供任务编排与取消机制（包装 QFuture） |
| 日志 | qDebug() | 提供分级/Sink 架构的工业级日志系统 |
| 网络 | QNetworkAccessManager | 提供拦截器/重试/会话管理/API 接口层 |

### 1.4 命名体系与未来扩展

采用 SoulXXX 命名空间体系，便于未来拆分：

| 库名 | 职责 |
|------|------|
| SoulCoreKit | 基础平台（本 PRD 范围） |
| SoulUI | UI 组件库（复杂控件、动画、主题） |
| SoulAI | AI 推理能力 |
| SoulRPC | RPC 框架 |
| SoulLSP | LSP 语言服务协议支持 |
| SoulMedia | 音视频处理 |
| SoulPlugin | 插件系统 |

## 二、模块总览（V1）

```text
SoulCoreKit
│
├── Core              // 基础设施（无第三方依赖）
├── Base              // 通用基类（所有业务对象的根）
├── Logging           // 日志系统（Sink 架构）
├── Configuration     // 配置管理（多文件分层）
├── Network           // 网络通信（含 API 层）
├── Storage           // 数据存储（含 Repository 接口）
├── Async             // 异步任务（包装 QFuture）
├── Event             // 事件总线（接口化）
├── UI                // UI 基础（含 Style 系统）
├── Utils             // 工具集合（子目录分类）
│
├── ThirdParty        // 第三方依赖（可选）
├── Tests             // 单元测试
├── Examples          // 示例程序
└── Docs              // 文档
```

## 三、详细模块规范

### 3.1 Core（基础设施）

- **依赖**：仅 Qt Core
- **职责**：最底层、无业务含义的基础能力。

| 组件 | 功能描述 |
|------|----------|
| Application | 封装 QApplication，提供生命周期管理、退出码、异常捕获。 |
| Platform | 运行时检测 OS/架构/运行时环境（开发/生产/测试）。 |
| Environment | 环境变量、命令行参数解析、工作目录管理。 |
| Version | 语义化版本号（SemVer 2.0）解析与比较。 |
| Uuid | 生成/解析 UUID（基于 QUuid 扩展）。 |
| Time | 时间戳、计时器、时区转换、可读格式化。 |
| Result\<T\> | Rust 风格的结果类型：Ok(T) / Err(Error)，支持链式操作。 |
| Error | 统一错误码（ErrorCode）+ 错误信息 + 嵌套支持。 |
| Interface | 纯虚接口基类（IInterface），便于 RTTI 与多态。 |
| Factory | 抽象工厂接口（IFactory\<T\>），支持按名称创建对象。 |
| Singleton | 线程安全的单例模板（控制析构顺序）。 |

> 移除：Logger、Config → 独立为 Logging、Configuration 模块。

### 3.2 Base（通用基类）

- **依赖**：Core
- **职责**：为所有业务模块提供统一的基类体系。

| 组件 | 功能描述 |
|------|----------|
| BaseObject | 继承自 QObject，提供调试名称、对象树管理、属性访问。 |
| BaseManager | 管理器基类（单例或全局访问），包含初始化/清理生命周期。 |
| BaseService | 服务基类（业务逻辑的容器，依赖注入准备）。 |
| BaseRepository | 仓库基类（配合 Storage 模块使用）。 |
| BaseWidget | UI 控件基类（集成 Style 系统）。 |
| BaseDialog | 对话框基类（统一布局、按钮、事件处理）。 |
| BaseViewModel | MVVM 模式中的 ViewModel 基类（数据绑定准备）。 |

**设计原则**：

- 所有业务项目的 XXXManager、XXXService 都应继承自这些基类。
- 提供默认实现（如 BaseManager 的 init()/shutdown() 钩子）。

### 3.3 Logging（日志系统）

- **依赖**：Core
- **职责**：工业级日志系统，支持多输出与灵活配置。

| 组件 | 功能描述 |
|------|----------|
| Logger | 日志门面（Facade），提供 debug()/info()/warn()/error()/fatal()。 |
| LogLevel | 日志级别枚举（Trace/Debug/Info/Warn/Error/Fatal）。 |
| ISink | 日志输出接口（ISink），支持自定义扩展。 |
| ConsoleSink | 输出到控制台（彩色支持）。 |
| FileSink | 输出到文件（支持大小滚动）。 |
| DailyFileSink | 按天滚动文件。 |
| CallbackSink | 回调函数输出（用于 UI 展示或远程上报）。 |
| CompositeSink | 组合多个 Sink。 |
| LogFormatter | 日志格式化器（时间/级别/文件/行号/消息）。 |

**使用示例**：

```cpp
SC_INFO("Network") << "Connected to server " << url;
SC_WARN("Storage") << "Cache miss for key " << key;
```

### 3.4 Configuration（配置管理）

- **依赖**：Core + Logging
- **职责**：分层配置管理，支持多文件与环境切换。

| 组件 | 功能描述 |
|------|----------|
| Config | 配置管理入口，支持加载/保存/监听变更。 |
| ConfigSource | 配置来源（文件/内存/远程）。 |
| JsonConfigSource | JSON 文件配置源。 |
| IniConfigSource | INI 文件配置源（可选）。 |
| ConfigSchema | 配置模式定义（支持类型验证与默认值）。 |

**配置目录结构（推荐）**：

```text
config/
  app.json          # 应用基础配置
  network.json      # 网络（超时/重试/代理）
  ui.json           # UI（字体/尺寸/默认主题）
  theme.json        # 主题（颜色/圆角/间距）
  logging.json      # 日志（级别/Sink 配置）
  [env]/            # 环境覆盖（dev/prod）
```

支持热加载：文件变更时自动重新加载（可选开关）。

### 3.5 Network（网络模块）

- **依赖**：Core + Logging + Configuration
- **职责**：完整的网络通信栈，从底层连接到业务 API。

| 组件 | 功能描述 |
|------|----------|
| HttpClient | 异步/同步 HTTP 客户端（支持超时/代理/SSL）。 |
| HttpRequest / HttpResponse | 请求/响应封装。 |
| HttpApi | 业务 API 接口层（自动序列化/反序列化）。 |
| Downloader | 断点续传下载器。 |
| Uploader | 多部分表单/大文件分片上传。 |
| WebSocket | WebSocket 客户端（自动重连/心跳）。 |
| TcpClient | 长连接 TCP 客户端（IM 等自定义协议）。 |
| RetryPolicy | 可配置重试策略（指数退避/固定间隔）。 |
| IInterceptor | 请求/响应拦截器接口（日志/认证/公共参数）。 |
| CookieJar | 内存/持久化 Cookie 管理。 |
| Session | 会话管理（自动携带认证 Token）。 |
| NetworkError | 网络错误分类与映射。 |

**HttpApi 设计（关键改进）**：

```cpp
// 业务层调用
auto api = sc::HttpApi<RestaurantEndpoint>(client);
api.get("/restaurants")
   .param("page", 1)
   .onSuccess([](const QJsonArray& data) { /* 处理 */ })
   .execute();
```

**吸收自 RPC 项目**：

- 连接池思想。
- 请求 ID → 回调映射。
- 序列化辅助（JSON ↔ 业务对象）。

### 3.6 Storage（存储模块）

- **依赖**：Core + Logging + Configuration
- **职责**：数据持久化，提供统一的 Repository 接口。

| 组件 | 功能描述 |
|------|----------|
| SqliteDatabase | SQLite 线程安全封装（WAL 模式/加密预留）。 |
| Settings | 用户设置（QSettings 替代品，统一 JSON 格式）。 |
| ICache\<K,V\> | 缓存接口（TTL/容量限制）。 |
| MemoryCache | LRU 内存缓存。 |
| DiskCache | 磁盘缓存（按 Key 存文件）。 |
| IRepository\<T\> | 仓库接口（业务实现此接口）。 |
| SqliteRepository\<T\> | SQLite 的 Repository 基类实现。 |
| MemoryRepository\<T\> | 内存中的 Repository（用于测试）。 |
| ISerializer | 序列化接口（JSON 实现）。 |

**IRepository 设计**：

```cpp
template <typename T>
class IRepository {
public:
    virtual sc::Result<T> findById(const QString& id) = 0;
    virtual sc::Result<std::vector<T>> findAll() = 0;
    virtual sc::Result<void> save(const T& entity) = 0;
    virtual sc::Result<void> remove(const QString& id) = 0;
};
```

> 不包含：ORM（对象关系映射）——手动写 SQL 更清晰可控。

### 3.7 Async（异步模块）

- **依赖**：Core + Logging
- **职责**：现代异步任务编排，包装 Qt6 的 QFuture/QPromise。

| 组件 | 功能描述 |
|------|----------|
| ThreadPool | 全局线程池（封装 QThreadPool）。 |
| Task | 异步任务基类（支持进度/取消/结果）。 |
| TaskRunner | 任务运行器（提交任务到线程池）。 |
| Dispatcher | 跨线程分发器（确保 UI 线程执行）。 |
| CancelableTask | 可取消任务（协作式取消）。 |
| Future\<T\> | 包装 QFuture\<T\>，支持 then()/onSuccess()/onFailure()。 |
| Promise\<T\> | 包装 QPromise\<T\>，与 Future 配对使用。 |

**设计决策**：

- 不重新发明 Future：直接使用 Qt6 的 QFuture/QPromise，提供更友好的链式 API 包装。
- 维护成本低，且能跟随 Qt 官方演进。

**使用示例**：

```cpp
auto future = sc::async([]() -> int {
    return heavyComputation();
}).then([](int result) {
    SC_INFO() << "Result: " << result;
});
```

### 3.8 Event（事件模块）

- **依赖**：Core + Logging
- **职责**：解耦模块间通信，提供灵活的事件总线。

| 组件 | 功能描述 |
|------|----------|
| IEventBus | 事件总线接口（便于 Mock 与替换）。 |
| DefaultEventBus | 默认实现（支持同步/异步派发）。 |
| IMessageBus | 消息总线接口（点对点/广播）。 |
| Subscription | 订阅句柄（用于取消订阅）。 |
| EventPriority | 事件优先级（高优先级可中止传递）。 |
| QtSignalAdapter | 将 Qt 信号转换为事件。 |

**关键设计**：

- 不是全局单例：通过依赖注入传递 IEventBus*，方便测试。
- 生命周期明确：事件总线由上层（如 Application）创建和管理。

```cpp
// App 层
auto bus = std::make_shared<DefaultEventBus>();
bus->subscribe<UserLoggedInEvent>([] (const auto& event) {
    SC_INFO() << "User logged in: " << event.username;
});
```

### 3.9 UI（界面基础模块）

- **依赖**：Core + Base + Logging + Configuration
- **职责**：UI 基础设施，统一样式与交互。

| 组件 | 功能描述 |
|------|----------|
| Style | 样式系统（颜色/圆角/Margin/Padding/字体）。 |
| Theme | 主题（Dark/Light），包含多套 Style。 |
| Icon | 图标系统（FontAwesome/SVG，支持动态颜色）。 |
| Dialog | 通用对话框（确认/提示/进度/输入框）。 |
| Toast | 轻提示（自动消失/位置可配）。 |
| Animation | 动画工具（淡入/滑动/缩放，基于 QPropertyAnimation）。 |
| Navigation | 页面导航栈（push/pop/replace）。 |
| Page | 页面基类（与 Navigation 配合，含生命周期）。 |
| Window | 主窗口基类（记忆尺寸/位置/状态）。 |
| Loading | 加载指示器（转圈/进度条/骨架屏）。 |
| EmptyWidget | 空状态展示（图标+文字+操作按钮）。 |

**Style vs Theme 分工**：

- **Style**：描述 UI 的"物理属性"（颜色值、圆角半径、字体大小、间距）。
- **Theme**：一组 Style 的集合（Dark Theme、Light Theme）。

业务代码访问：`Style::instance()->color(Colors::Primary)`。

### 3.10 Utils（工具集合）

- **依赖**：Core
- **职责**：收纳高频但零散的工具函数，按子目录分类，避免单文件膨胀。

```text
utils/
├── json/          // JSON 读写辅助
├── file/          // 文件/路径操作
├── string/        // 字符串工具
├── crypto/        // MD5/SHA/AES
├── image/         // 图像缩放/裁剪/格式转换
├── process/       // 进程管理
├── clipboard/     // 剪贴板操作
├── datetime/      // 日期时间格式化
├── compress/      // ZIP/GZIP 压缩
└── xml/           // XML 读写辅助
```

> 命名空间：所有工具函数放入 `sc::utils::XXX` 子命名空间。

### 3.11 ThirdParty（第三方库）

原则：尽量减少外部依赖，优先 Qt 原生。

可选引入（打包在 ThirdParty 中）：

- nlohmann/json（若 Qt JSON 性能不足）
- spdlog（若自研 Logger 不满足需求）
- QuaZip（ZIP 压缩）
- SQLiteCpp（若 Qt SQL 封装不够）

> 决策规则：若 Qt 原生能满足 80% 需求，则不用第三方库。

## 四、非功能需求

| 类别 | 要求 |
|------|------|
| 性能 | 模块启动不影响应用启动速度（懒加载），内存占用 < 10MB（基础模块）。 |
| 稳定性 | 核心模块（Core/Base/Logging）单元测试覆盖率 > 90%。 |
| 文档 | 每个公开类/函数有 Doxygen 注释；提供《快速上手》与《架构设计》文档。 |
| 兼容性 | Windows 10/11、macOS 12+、Ubuntu 22.04+；MSVC 2022 / Clang 14+ / GCC 11+。 |
| 可维护性 | 头文件 public/private 分离；CMake 构建；模块间循环依赖检查。 |
| 代码规模 | V1 总代码量控制在 5 万～8 万行。 |

## 五、演进路线图

### V1.0（当前）

- 完成上述 11 个模块（Core、Base、Logging、Configuration、Network、Storage、Async、Event、UI、Utils、ThirdParty）。
- 提供 2 个 Demo 应用验证。
- API 文档与快速入门指南。

### V1.1（按需添加）

- Plugin：若 SoulIDE 需要插件系统。
- Command：若多个项目需要撤销/重做（封装 QUndoStack）。

### V2.0（长期）

- SoulRPC：吸收现有 RPC 项目，独立为 SoulRPC 库。
- SoulLSP：若 IDE 需要 LSP 支持。
- SoulAI：若 AI 能力在多个项目中复用。

## 六、验收标准

- **编译**：所有模块在三大平台（Win/macOS/Linux）编译无警告。
- **测试**：核心模块单元测试覆盖率 > 80%，集成测试覆盖 Network + Storage 典型场景。
- **示例**：至少 2 个 Demo：
  - Demo 1：纯逻辑应用（网络请求 + 数据存储 + 日志）。
  - Demo 2：带 UI 的桌面应用（主题切换 + 事件总线 + 异步任务）。
- **文档**：README + 快速开始 + 完整的 Doxygen API 参考。
- **打包**：生成静态库/动态库，便于其他项目链接。

## 七、总结

SoulCoreKit 的定位是你自己的 Qt 应用基础设施，而非通用的应用程序框架。它遵循"两次原则"，V1 只做已验证的通用能力，为未来自然演进留出空间。