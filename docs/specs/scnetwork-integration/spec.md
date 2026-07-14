# SCNetwork 与 SoulCoreKit 整合方案 - 产品需求文档

## Overview

- **Summary**: 将 SCNetwork 网络框架整合到 SoulCoreKit 中，实现协议无关的统一网络抽象层，使业务层无需关心底层通信协议，支持 HTTP/TCP/WebSocket 的统一接口调用。
- **Purpose**: 解决现有网络模块缺乏统一抽象、业务层直接依赖具体协议实现、难以扩展新协议的问题，构建企业级 Qt 网络基础设施。
- **Target Users**: SoulCoreKit 的开发者和使用者，包括业务层开发者、框架维护者、测试工程师。

## Goals

1. 定义 `INetwork` 统一接口，实现协议无关的网络通信抽象
2. 统一 Request/Response 对象模型，消除协议差异
3. 统一网络状态机和事件模型，简化业务层状态管理
4. 实现策略模式（重试/心跳/重连/超时），支持策略组合
5. 扩展拦截器链，支持日志/鉴权/签名等横切关注点
6. 提供工厂模式，实现网络实例的统一创建
7. 保持与现有 SoulCoreKit 架构规范和设计原则的完全契合

## Non-Goals (Out of Scope)

1. **不重写现有实现**: 不修改现有 `HttpClient`/`TcpClient`/`WebSocket` 的核心逻辑
2. **不引入新依赖**: 不引入除 Qt6 和现有 SoulCoreKit 模块以外的第三方库
3. **不修改公共 API**: 不破坏现有公共接口的向后兼容性
4. **不涉及 UI**: 网络模块不包含任何 UI 代码
5. **MQTT/Bluetooth/SerialPort**: v1.0 暂不支持，留待后续版本

## Background & Context

### 现有架构分析

SoulCoreKit 采用分层架构，网络模块位于 Infrastructure Layer：

```
UI Layer → Business Layer → Infrastructure Layer → Foundation Layer
```

现有网络模块 (`soul/network/`) 已实现：
- `HttpClient`: 同步/异步 HTTP 请求，支持拦截器和重试
- `TcpClient`: TCP 长连接客户端，支持自动重连
- `WebSocket`: WebSocket 客户端，支持自动重连和心跳
- `HttpRequest`/`HttpResponse`: HTTP 专用请求响应模型
- `IInterceptor`: HTTP 拦截器接口
- `RetryPolicy`: 重试策略

### 核心问题

1. **缺乏统一接口**: 业务层直接依赖 `HttpClient`/`TcpClient`/`WebSocket` 具体类
2. **协议耦合**: 业务代码需要判断协议类型，无法无缝切换
3. **模型不一致**: `HttpRequest`/`HttpResponse` 只适用于 HTTP
4. **状态管理分散**: 各协议状态独立，业务层需要分别处理
5. **策略能力有限**: 只有 `RetryPolicy`，缺乏心跳、重连、超时等策略
6. **拦截器不通用**: `IInterceptor` 只支持 HTTP

### SoulCoreKit 设计原则约束

必须遵循以下核心原则：
1. **Architecture First**: 不破坏现有架构
2. **Interface First**: 先定义接口，再实现
3. **Single Responsibility**: 每个类只负责一件事
4. **Minimal Change**: 最小化改动，优先复用现有代码
5. **依赖规则**: Network 只能依赖 Core 和 Logging

### Qt 技术约束

1. **Q_OBJECT 不能用于纯虚接口**: Qt 的 MOC 机制要求 `Q_OBJECT` 宏不能用于纯虚抽象类。解决方案是分离接口和信号基类：
   - `INetwork`: 纯虚接口（继承 `IInterface`，无 `Q_OBJECT`）
   - `NetworkBase`: Qt 信号基类（继承 `QObject`，包含信号定义）
   - 适配器同时继承两者

2. **协议差异**: HTTP 是请求-响应模式，TCP/WebSocket 是流式/消息模式。需要设计协议中立的模型：
   - 通用 `NetworkMessage` 作为协议中立的消息载体
   - `HttpRequest`/`HttpResponse` 保持现有设计，通过适配器转换

## Functional Requirements

### FR-1: 统一网络接口 INetwork

定义 `INetwork` 纯虚接口，继承自 `IInterface`，**不含 Q_OBJECT 和信号定义**：

- 连接管理：`connectTo()`/`disconnect()`/`isConnected()`
- 消息发送：`send()`（同步）/`sendAsync()`（异步）
- 状态查询：`state()`
- 策略配置：`setPolicy()`
- 拦截器配置：`addInterceptor()`
- 返回 `Result<NetworkMessage>` 作为统一响应

### FR-2: 网络信号基类 NetworkBase

定义 `NetworkBase` 类，继承自 `QObject`，**包含统一事件信号**：

```cpp
class NetworkBase : public QObject {
    Q_OBJECT
signals:
    void connected();
    void disconnected();
    void received(const NetworkMessage& msg);
    void error(const NetworkError& error);
    void progress(qint64 current, qint64 total);
    void timeout();
    void reconnect(int attempt);
};
```

### FR-3: 统一消息模型 NetworkMessage

定义协议中立的 `NetworkMessage` 作为消息载体：

```cpp
struct NetworkMessage {
    QString api;                    // 接口标识（可选）
    QByteArray payload;             // 消息体（协议中立）
    QVariantMap metadata;           // 元数据（协议特定信息）
    int statusCode = 0;             // 状态码（HTTP专用）
    QString message;                // 消息描述
    qint64 duration = 0;            // 耗时(ms)
    NetworkError error;             // 错误信息
};
```

**协议映射规则**：
- **HTTP**: `payload` = body, `metadata` = headers, `statusCode` = HTTP状态码
- **TCP**: `payload` = 原始数据, `metadata` = { "packetType", "sequence" }
- **WebSocket**: `payload` = 消息内容, `metadata` = { "messageType": "text" | "binary" }

### FR-4: 统一状态机

定义 `NetworkState` 枚举，统一所有协议的生命周期：

```
Created → Connecting → Connected → Working → Reconnecting → Disconnected → Destroyed
```

### FR-5: 策略层

定义策略接口 `INetworkPolicy`，支持策略组合：

- `INetworkPolicy`: 策略接口，包含 `apply()` 方法
- `RetryPolicy`: 迁移现有实现到策略模块
- `HeartbeatPolicy`: TCP/WebSocket 心跳策略
- `ReconnectPolicy`: 重连策略
- `TimeoutPolicy`: 超时策略

### FR-6: 拦截器链扩展

扩展 `IInterceptor` 为通用接口，支持所有协议：

- 请求拦截：`onRequest(NetworkMessage&)`
- 响应拦截：`onResponse(NetworkMessage&)`
- 内置拦截器：LoggingInterceptor、AuthInterceptor、TokenInterceptor

### FR-7: 工厂模式

基于现有 `Factory<T>` 模板，实现 `NetworkFactory`：

- 按协议名称创建实例：`NetworkFactory::create("http")`
- 支持注册自定义协议实现
- 返回 `std::unique_ptr<INetwork>`

### FR-8: 适配器模式

适配器类同时继承 `INetwork` 和 `NetworkBase`，使现有实现无需修改即可适配新接口：

- `HttpClientAdapter`: 适配 `HttpClient`，实现数据模型转换
- `TcpClientAdapter`: 适配 `TcpClient`，处理流式数据
- `WebSocketAdapter`: 适配 `WebSocket`，处理文本/二进制消息

### FR-9: 构建系统配置

更新 `CMakeLists.txt`，添加所有新增文件的编译配置：

- 新增 `soul_network_core` 子模块（INetwork、NetworkMessage、NetworkState）
- 新增 `soul_network_policy` 子模块（策略层）
- 新增 `soul_network_factory` 子模块（工厂层）
- 更新 `soul_network` 主模块，包含所有子模块

## Non-Functional Requirements

### NFR-1: 向后兼容性

- 现有 API 保持不变，新增接口和适配器
- 支持渐进式迁移，业务层可按需切换
- 旧头文件路径仍可使用（通过 `[[deprecated]]` 标记）

### NFR-2: 线程安全

- 所有新增组件必须线程安全
- 遵循现有 `soul/network/` 的线程安全规范

### NFR-3: 性能

- 适配器层引入的开销不超过 1%
- 不影响现有网络操作的响应时间

### NFR-4: 可测试性

- 所有接口支持 Mock
- 提供单元测试和集成测试

### NFR-5: 日志集成

- 遵循现有 `Logger` 框架
- 使用 `SC_INFO`/`SC_WARN`/`SC_ERROR` 宏

## Constraints

### Technical

- **语言**: C++17，Qt6
- **命名空间**: `sc::network`
- **接口规范**: `INetwork` 继承 `IInterface`，实现 `interfaceName()`
- **错误处理**: 使用 `Result<T>` 和 `Error`
- **依赖规则**: 只能依赖 `soul_core` 和 `soul_logging`
- **Qt 约束**: `Q_OBJECT` 不能用于纯虚接口，必须分离接口和信号基类

### Business

- **最小改动**: 不修改现有公共接口签名
- **渐进式迁移**: 允许新旧 API 共存
- **代码规范**: 遵循现有命名和导出约定

### Dependencies

- `soul_core`: Result、Error、IInterface、Factory
- `soul_logging`: Logger
- `Qt6Network`: QNetworkAccessManager、QTcpSocket、QWebSocket

## Assumptions

1. 现有 `HttpClient`/`TcpClient`/`WebSocket` 核心逻辑正确，无需重写
2. 业务层开发者愿意逐步迁移到新接口
3. Qt6 环境已正确配置
4. 现有测试框架可用于新增组件的测试
5. 协议特定信息可通过 `NetworkMessage::metadata` 传递

## Acceptance Criteria

### AC-1: INetwork 接口定义

- **Given**: SoulCoreKit 项目环境已就绪
- **When**: 查看 `include/soul/network/core/inetwork.h`
- **Then**: 接口继承 `IInterface`，不含 `Q_OBJECT`，包含 `connectTo()`/`disconnect()`/`send()`/`sendAsync()`/`state()`/`setPolicy()`/`addInterceptor()` 方法
- **Verification**: `human-judgment`

### AC-2: NetworkBase 信号基类

- **Given**: 查看 `include/soul/network/core/network_base.h`
- **When**: 检查信号定义
- **Then**: 继承 `QObject`，包含 `Q_OBJECT` 宏，定义 connected()/disconnected()/received()/error()/progress()/timeout()/reconnect() 信号
- **Verification**: `human-judgment`

### AC-3: NetworkMessage 统一消息模型

- **Given**: 查看 `include/soul/network/core/network_message.h`
- **When**: 检查模型字段
- **Then**: 包含 `api`、`payload`、`metadata`、`statusCode`、`message`、`duration`、`error` 字段
- **Verification**: `human-judgment`

### AC-4: 统一状态机

- **Given**: 查看 `include/soul/network/core/network_state.h`
- **When**: 检查状态枚举
- **Then**: 包含 Created、Connecting、Connected、Working、Reconnecting、Disconnected、Destroyed 7 个状态
- **Verification**: `human-judgment`

### AC-5: 适配器实现

- **Given**: 现有 `HttpClient`/`TcpClient`/`WebSocket` 已实现
- **When**: 创建适配器类并测试
- **Then**: 适配器同时继承 `INetwork` 和 `NetworkBase`，正确调用底层实现并转换数据模型
- **Verification**: `programmatic`（单元测试）

### AC-6: NetworkFactory 创建网络实例

- **Given**: `NetworkFactory` 已注册 HTTP/TCP/WebSocket 协议
- **When**: 调用 `NetworkFactory::create("http")`
- **Then**: 返回 `std::unique_ptr<INetwork>` 指向正确的协议实现
- **Verification**: `programmatic`（单元测试）

### AC-7: 策略组合

- **Given**: 配置了 `RetryPolicy` 和 `TimeoutPolicy`
- **When**: 发送请求
- **Then**: 请求按策略执行，超时和重试行为符合预期
- **Verification**: `programmatic`（单元测试）

### AC-8: 拦截器链执行

- **Given**: 添加了 `LoggingInterceptor` 和 `AuthInterceptor`
- **When**: 发送请求并接收响应
- **Then**: 请求按顺序经过所有拦截器，响应按逆序经过所有拦截器
- **Verification**: `programmatic`（单元测试）

### AC-9: 协议特定信息传递

- **Given**: 通过 WebSocket 发送文本和二进制消息
- **When**: 接收 `received()` 信号
- **Then**: `NetworkMessage::metadata` 正确包含 `messageType` 字段（"text" 或 "binary"）
- **Verification**: `programmatic`（单元测试）

### AC-10: 向后兼容性

- **Given**: 现有业务代码使用 `HttpClient` 直接调用
- **When**: 升级框架后运行现有代码
- **Then**: 代码正常工作，无编译错误和运行时错误
- **Verification**: `programmatic`（回归测试）

### AC-11: 构建系统

- **Given**: 更新 `CMakeLists.txt`
- **When**: 编译项目
- **Then**: 所有新增文件正确编译，静态库构建成功
- **Verification**: `programmatic`（编译验证）

### AC-12: 架构契合度

- **Given**: 查看新增代码的依赖关系
- **When**: 检查模块依赖和代码规范
- **Then**: 符合现有架构约束（只依赖 Core 和 Logging），遵循命名规范和导出约定
- **Verification**: `human-judgment`

## Open Questions

- [ ] 是否需要支持双向流（TCP/WebSocket 的持续数据传输）的统一接口？
- [ ] `NetworkMessage` 的序列化方式如何与现有 `JsonSerializer` 整合？
- [ ] 监控指标（QPS/RT/成功率）是否需要在 v1.0 中实现，还是延后到 v1.6？
- [ ] 连接池（Connection Pool）是否需要在 v1.0 中实现，还是延后到 v1.5？
- [ ] 是否需要定义协议枚举（`NetworkProtocol::Http`/`Tcp`/`WebSocket`）用于类型判断？
