# SCNetwork 与 SoulCoreKit 整合方案 - 验证检查清单

## 架构契合度
- [x] 新增代码不破坏现有架构，遵循 Architecture First 原则
- [x] Network 模块只依赖 Core 和 Logging，无循环依赖
- [x] 所有接口继承 `IInterface`，实现 `interfaceName()`
- [x] 错误处理使用 `Result<T>` 和 `Error`
- [x] 命名规范符合 SoulCoreKit 约定（PascalCase 类名，snake_case 文件名）

## Qt 技术约束
- [x] `INetwork` 接口不含 `Q_OBJECT` 宏，纯虚函数定义正确
- [x] `NetworkBase` 继承 `QObject`，包含 `Q_OBJECT` 宏和信号定义
- [x] 适配器类同时继承 `INetwork` 和 `NetworkBase`，MOC 生成正确
- [x] 信号参数使用 `NetworkMessage` 和 `NetworkError`，类型一致

## 接口设计
- [x] `INetwork` 接口定义完整，包含所有必要方法
- [x] `NetworkBase` 包含统一事件信号定义（connected/disconnected/received/error/progress/timeout/reconnect）
- [x] `NetworkState` 枚举包含 7 个状态，顺序符合生命周期
- [x] `NetworkMessage` 模型字段完整（api/payload/metadata/statusCode/message/duration/error）
- [x] 策略接口 `INetworkPolicy` 设计合理，支持组合

## 协议中立性
- [x] `NetworkMessage` 不包含协议特定字段（HTTP/TCP/WebSocket）
- [x] 协议特定信息通过 `metadata` 传递
- [x] HTTP 映射规则正确（payload=body, metadata=headers, statusCode=HTTP状态码）
- [x] TCP 映射规则正确（payload=原始数据, metadata={packetType, sequence}）
- [x] WebSocket 映射规则正确（payload=消息内容, metadata={messageType}）

## 适配器实现
- [x] `HttpClientAdapter` 正确实现 `INetwork` 和 `NetworkBase`
- [x] `TcpClientAdapter` 正确实现 `INetwork` 和 `NetworkBase`
- [x] `WebSocketAdapter` 正确实现 `INetwork` 和 `NetworkBase`
- [x] 适配器正确转换数据模型（`NetworkMessage` ↔ 协议特定模型）
- [x] 适配器正确转发事件信号
- [x] `WebSocketAdapter` 的 `metadata` 正确包含 `messageType` 字段
- [x] 适配器实现拦截器链（请求顺序执行、响应逆序执行）
- [x] 适配器实现策略应用逻辑

## 工厂模式
- [x] `NetworkFactory` 基于现有 `Factory<T>` 模板实现
- [x] 支持按协议名称创建实例（http/tcp/ws）
- [x] 支持注册自定义协议实现
- [x] 创建未知协议返回 nullptr

## 策略层
- [x] `RetryPolicy` 迁移到策略模块，实现 `INetworkPolicy` 接口
- [x] `TimeoutPolicy` 实现正确，能设置请求超时
- [x] 策略支持组合应用到请求
- [x] 保持向后兼容性（旧头文件路径仍可使用）

## 拦截器链
- [x] `IInterceptor` 扩展为通用接口，支持所有协议
- [x] `LoggingInterceptor` 实现正确，记录请求/响应日志
- [x] `AuthInterceptor` 实现正确，注入认证信息
- [x] 拦截器链按顺序执行请求拦截，按逆序执行响应拦截

## 向后兼容性
- [x] 现有 `HttpClient`/`TcpClient`/`WebSocket` API 保持不变
- [x] 旧头文件路径仍可使用（通过 `[[deprecated]]` 标记）
- [x] 现有测试套件全部通过
- [x] 示例程序 `network_example.exe` 正常运行

## 构建系统
- [x] `CMakeLists.txt` 更新，包含所有新增目录和文件
- [x] `soul_network_core` 子模块配置正确
- [x] `soul_network_policy` 子模块配置正确
- [x] `soul_network_factory` 子模块配置正确
- [x] 项目编译通过，无错误和警告
- [x] 静态库 `libsoul_network.a` 构建成功

## 测试覆盖
- [x] `INetwork` 接口测试通过
- [x] `NetworkFactory` 测试通过
- [x] 适配器测试通过
- [x] 策略组合测试通过
- [x] 拦截器链测试通过
- [x] `NetworkMessage` 转换测试通过
- [x] 协议特定信息传递测试通过（WebSocket text/binary）
- [x] 回归测试通过
- [x] 测试覆盖率 >80%

## 文档与导出
- [x] 更新 `docs/03_module_specification.md`，添加 SCNetwork 模块说明
- [x] 创建 `include/soul/network/soul_network.h` 统一导出头文件
- [x] 所有新增类包含 Doxygen 文档
- [x] 头文件遵循 `#ifndef SOUL_NETWORK_XXX_H` 格式

## 编译与链接
- [x] 项目编译通过，无错误和警告
- [x] 所有模块链接正常
- [x] Qt MOC 生成正确（包含 `Q_OBJECT` 宏的类）
- [x] 静态库构建成功