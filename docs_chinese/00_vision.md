# SoulCoreKit 愿景

## 项目概述

SoulCoreKit 是一个轻量级、模块化、演进驱动的 C++/Qt 基础设施库，专为 **CS（Client-Server）架构**应用开发设计。它作为所有 CS 风格项目（IDE、音乐播放器、电台、笔记本、IM、EatDecider 等）的基础，提供一致的、生产级别的组件和工具。脚手架设计借鉴 SpringBoot 的 IoC/DI/AOP/生命周期等理念，但架构是 CS（Qt 桌面客户端 + Linux 服务端），而非 BS（Browser-Server）。

## 使命

提供**稳定、可维护、久经考验**的基础设施层，使高质量 Qt CS 架构应用能够快速开发——Client 端在 Windows/macOS/Linux 桌面运行，Server 端在 Linux（Ubuntu 20.04+）或云服务器上运行——同时保持严格的架构完整性和开发体验。

## 核心价值观

### Native First（原生优先）
- 遵循 Qt 官方推荐的实践
- 不重复实现 Qt 已成熟的能力
- 利用 Qt 的原生性能和平台集成

### Composition over Inheritance（组合优于继承）
- 优先使用组合模式而非深层继承层次
- 提升灵活性，避免紧耦合

### Interface First（接口优先）
- 在实现之前通过接口定义公共能力
- 支持依赖注入和可测试性
- 允许同一接口有多个实现

### Minimal Dependency（最小依赖）
- 模块间保持最小依赖
- 禁止循环依赖
- 保持库的轻量级和专注性

### Evolution over Prediction（演进优于预测）
- 遵循"两次原则"：只添加至少被两个项目验证过的功能
- 避免过度设计和投机性设计
- 优先采用经过验证的解决方案而非预测未来需求

## 定位

| 方面 | SoulCoreKit | Qt |
|------|-------------|-----|
| **范围** | CS 架构应用基础设施 | 应用框架 |
| **焦点** | 跨项目 CS 工具和模式 | 核心平台能力 |
| **理念** | 演进驱动、最小化、SpringBoot 风格脚手架 | 全面、功能丰富 |
| **关系** | 互补 | 基础 |

## 架构

SoulCoreKit 是 **CS（Client-Server）架构**脚手架，不是 BS（Browser-Server）框架：

- **Client 端**：Windows/macOS/Linux 上的 Qt 桌面应用，使用 SoulCoreKit + SoulCoreKitUi
- **Server 端**：Linux（Ubuntu 20.04+）或云服务器上的后台进程，使用 SoulCoreKit（不含 UI）
- **通信方式**：Client 与 Server 之间通过 HTTP/TCP/WebSocket/RPC 通信
- **脚手架风格**：借鉴 SpringBoot 的 IoC/DI/AOP/模块生命周期等理念，但不对标 Servlet/Tomcat/Filter/MVC 等 BS 专属概念

## 目标用例

- **CS 桌面应用开发**：构建 Qt 桌面客户端 + Linux 服务端的 CS 架构应用
- **跨项目代码复用**：在 CS 项目间共享经过验证的组件
- **快速原型**：从坚实基础开始，专注于业务逻辑
- **企业级应用**：具备测试和文档的生产就绪组件

## 长期愿景

SoulCoreKit 旨在演变为一系列专业库：

| 库 | 焦点 |
|----|------|
| **SoulCoreKit** | 核心基础设施（当前范围，CS 双端共享） |
| **SoulUI** | UI 组件、动画、主题（仅 Client 端） |
| **SoulAI** | AI 推理能力 |
| **SoulRPC** | RPC 框架（CS 通信） |
| **SoulLSP** | LSP 协议支持 |
| **SoulMedia** | 音视频处理 |
| **SoulPlugin** | 插件系统 |

## 成功标准

- **API 稳定性**：公共 API 在次要版本间保持兼容
- **测试覆盖率**：核心模块达到 >90% 的单元测试覆盖率
- **平台支持**：Client 端 Windows/macOS/Linux；Server 端 Linux（Ubuntu 20.04+）/云服务器
- **文档**：完整的 API 参考和使用指南
- **社区**：开放外部贡献，有清晰的指南
- **性能**：最小启动开销，60fps 流畅动画