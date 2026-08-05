# 01 — 项目概览

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 1.1 项目名片

| 属性 | 值 |
|------|-----|
| **项目名称** | SoulCoreKit |
| **当前版本** | v2.5.0 |
| **定位** | Qt CS 架构 · SpringBoot 风格脚手架 |
| **一句话定义** | 面向 Qt/C++ 的模块化应用开发框架，借鉴 Spring Boot 的分层架构、模块化、约定优于配置和生命周期管理理念，为 C/S 应用提供统一的基础设施与业务架构能力。 |
| **简称** | Qt Application Framework |
| **语言标准** | C++17 |
| **框架依赖** | Qt 6.5.3 LTS |
| **第三方库** | nlohmann/json 3.11.3, spdlog 1.14.1, amqpcpp 4.3.27 (可选) |
| **平台** | Windows / Linux (Ubuntu 20.04+) / macOS |
| **编译器** | GCC 11 / MSVC 2019 / Clang 14 |
| **许可证** | MIT |

## 1.2 核心价值

### Native First
- 拥抱 Qt 官方推荐实践
- 避免重新实现 Qt 已有的成熟能力
- 利用 Qt 原生性能和平台集成

### Composition over Inheritance
- 优先组合模式而非深层继承层次
- 促进灵活性，避免紧耦合

### Interface First
- 通过接口定义公共能力，再实现
- 启用依赖注入和可测试性
- 同一接口允许多种实现

### Minimal Dependency
- 保持模块间最小依赖
- 禁止循环依赖
- 保持库轻量聚焦

### Evolution over Prediction
- 遵循 "Two-Time Rule"：仅添加至少 2 个项目验证过的特性
- 避免过度工程和推测性设计
- 优先经验证的方案而非未来预测

## 1.3 定位对比

| 维度 | SoulCoreKit | Qt |
|------|-------------|-----|
| **范围** | CS 架构应用基础设施 | 应用框架 |
| **焦点** | 跨项目 CS 工具和模式 | 核心平台能力 |
| **哲学** | 演进驱动、极简、SpringBoot 风格 | 全面、功能丰富 |
| **关系** | 互补 | 基础 |

## 1.4 与 SpringBoot 的关系

- **借鉴**: 分层架构、IoC/DI、生命周期管理、约定优于配置、Actuator 端点
- **不复制**: Servlet/Tomcat/Filter/MVC HTTP 请求处理链
- **适配 CS**: Signal/Slot 替代 HTTP Request/Response，Page 导航替代 URL 路由

## 1.5 项目规模

| 指标 | 数值 |
|------|------|
| 模块数量 | 27 个 |
| 头文件数量 | 200+ |
| 源文件数量 | 150+ |
| 测试文件数量 | ~50 个 |
| UI 组件数量 | 30+ |
| CI/CD 工作流 | 5 个 |
| 规范文档 | 15+ 篇 |
| 根 CMake 行数 | 600+ |
| 子模块 CMake 文件 | 30 个 |

## 1.6 长期愿景

SoulCoreKit 旨在演进为一个专业库家族：

| 库 | 焦点 |
|----|------|
| **SoulCoreKit** | 核心基础设施（当前范围，CS 共享） |
| **SoulUI** | UI 组件、动画、主题（Client 端） |
| **SoulAI** | AI 推理能力 |
| **SoulRPC** | RPC 框架（CS 通信） |
| **SoulLSP** | LSP 协议支持 |
| **SoulMedia** | 音视频处理 |
| **SoulPlugin** | 插件系统 |