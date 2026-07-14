# SCNetwork 与 SoulCoreKit 整合方案 - 实现计划

## [x] Task 1: 定义 INetwork 统一接口
- **Priority**: high
- **Depends On**: None
- **Description**: 
  - 创建 `include/soul/network/core/inetwork.h`，定义纯虚接口 `INetwork`
  - 接口继承 `IInterface`，**不含 Q_OBJECT**，实现 `interfaceName()`
  - 包含连接管理、消息发送、状态查询、策略配置、拦截器配置等方法
  - 返回 `Result<NetworkMessage>` 作为统一响应
- **Acceptance Criteria Addressed**: AC-1

## [x] Task 2: 定义 NetworkBase 信号基类
- **Priority**: high
- **Depends On**: Task 3
- **Description**: 
  - 创建 `include/soul/network/core/network_base.h`，定义 `NetworkBase` 类
  - 继承 `QObject`，包含 `Q_OBJECT` 宏
  - 定义统一事件信号：connected()/disconnected()/received()/error()/progress()/timeout()/reconnect()
- **Acceptance Criteria Addressed**: AC-2

## [x] Task 3: 定义 NetworkMessage 统一消息模型
- **Priority**: high
- **Depends On**: None
- **Description**: 
  - 创建 `include/soul/network/core/network_message.h`，定义通用 `NetworkMessage` 结构体
  - 包含: api、payload、metadata、statusCode、message、duration、error
  - 提供协议特定的构造函数和转换方法
- **Acceptance Criteria Addressed**: AC-3

## [x] Task 4: 定义统一状态枚举 NetworkState
- **Priority**: high
- **Depends On**: None
- **Description**: 
  - 创建 `include/soul/network/core/network_state.h`
  - 定义 `NetworkState` 枚举：Created、Connecting、Connected、Working、Reconnecting、Disconnected、Destroyed
- **Acceptance Criteria Addressed**: AC-4

## [x] Task 5: 定义策略接口 INetworkPolicy
- **Priority**: medium
- **Depends On**: Task 3
- **Description**: 
  - 创建 `include/soul/network/policy/inetwork_policy.h`
  - 定义 `INetworkPolicy` 接口，包含 `apply(NetworkMessage&)` 方法
  - 创建 `include/soul/network/policy/timeout_policy.h`
- **Acceptance Criteria Addressed**: AC-7

## [x] Task 6: 迁移 RetryPolicy 到策略模块
- **Priority**: medium
- **Depends On**: Task 5
- **Description**: 
  - 将现有 `retry_policy.h` 从 `include/soul/network/` 移动到 `include/soul/network/policy/`
  - 修改 `RetryPolicy` 实现 `INetworkPolicy` 接口
  - 在旧位置创建 `retry_policy.h` 转发头文件，使用 `[[deprecated]]` 标记
- **Acceptance Criteria Addressed**: AC-7

## [x] Task 7: 扩展 IInterceptor 为通用接口
- **Priority**: medium
- **Depends On**: Task 3
- **Description**: 
  - 修改现有 `include/soul/network/i_interceptor.h`，使其支持通用 `NetworkMessage`
  - 新增 `include/soul/network/interceptor/logging_interceptor.h`
  - 新增 `include/soul/network/interceptor/auth_interceptor.h`
- **Acceptance Criteria Addressed**: AC-8

## [x] Task 8: 实现 HttpClientAdapter
- **Priority**: high
- **Depends On**: Task 1, Task 2, Task 3, Task 7
- **Description**: 
  - 创建 `include/soul/network/http/http_client_adapter.h`
  - 同时继承 `INetwork` 和 `NetworkBase`
  - 内部持有 `HttpClient`，实现数据模型转换（`NetworkMessage` ↔ `HttpRequest`/`HttpResponse`）
  - 实现拦截器链和策略应用逻辑
- **Acceptance Criteria Addressed**: AC-5

## [x] Task 9: 实现 TcpClientAdapter
- **Priority**: high
- **Depends On**: Task 1, Task 2, Task 3
- **Description**: 
  - 创建 `include/soul/network/tcp/tcp_client_adapter.h`
  - 同时继承 `INetwork` 和 `NetworkBase`
  - 内部持有 `TcpClient`，实现数据模型转换和事件转发
  - 在 `metadata` 中添加 `packetType` 和 `sequence` 信息
  - 实现拦截器链和策略应用逻辑
- **Acceptance Criteria Addressed**: AC-5

## [x] Task 10: 实现 WebSocketAdapter
- **Priority**: high
- **Depends On**: Task 1, Task 2, Task 3
- **Description**: 
  - 创建 `include/soul/network/websocket/ws_client_adapter.h`
  - 同时继承 `INetwork` 和 `NetworkBase`
  - 内部持有 `WebSocket`，实现数据模型转换和事件转发
  - 在 `metadata` 中添加 `messageType` 字段（"text" 或 "binary"）
  - 实现拦截器链和策略应用逻辑
- **Acceptance Criteria Addressed**: AC-5, AC-9

## [x] Task 11: 实现 NetworkFactory
- **Priority**: high
- **Depends On**: Task 1, Task 8, Task 9, Task 10
- **Description**: 
  - 创建 `include/soul/network/factory/network_factory.h`
  - 基于现有 `Factory<T>` 模板实现
  - 注册 HTTP/TCP/WebSocket 协议的创建函数
- **Acceptance Criteria Addressed**: AC-6

## [x] Task 12: 更新 CMakeLists.txt
- **Priority**: high
- **Depends On**: 所有新增文件任务
- **Description**: 
  - 更新项目根目录 `CMakeLists.txt`
  - 为新增的 `soul/network/core/`、`soul/network/policy/`、`soul/network/interceptor/`、`soul/network/factory/` 目录添加编译配置
  - 更新 `soul_network` 目标，包含所有新增源文件
- **Acceptance Criteria Addressed**: AC-11

## [x] Task 13: 更新模块文档和头文件包含
- **Priority**: medium
- **Depends On**: 所有功能任务
- **Description**: 
  - 更新 `docs/03_module_specification.md`，添加 SCNetwork 模块说明
  - 创建 `include/soul/network/soul_network.h` 统一导出头文件
- **Acceptance Criteria Addressed**: AC-12

## [x] Task 14: 编写单元测试
- **Priority**: medium
- **Depends On**: 所有功能任务
- **Description**: 
  - 创建 `tests/test_network.cpp`，测试 `INetwork` 接口、`NetworkFactory`、适配器、策略组合、拦截器链
- **Acceptance Criteria Addressed**: AC-5, AC-6, AC-7, AC-8, AC-9

## [x] Task 15: 回归测试（向后兼容性）
- **Priority**: high
- **Depends On**: 所有功能任务
- **Description**: 
  - 运行现有测试套件
  - 确保现有代码（直接使用 `HttpClient`/`TcpClient`/`WebSocket`）正常工作
- **Acceptance Criteria Addressed**: AC-10