# SoulCoreKit Vision

## Project Overview

SoulCoreKit 是一个面向 Qt/C++ 的模块化应用开发框架，借鉴 Spring Boot 的分层架构、模块化、约定优于配置和生命周期管理理念，为 C/S 应用提供统一的 Module、Controller、Service、ViewModel、Router 与基础设施能力，并通过 Web Adapter 为未来 B/S UI 扩展提供基础。

**简称**: Qt Application Framework（而非 "Qt Utility Library"）

## Mission

To provide a **stable, maintainable, and battle-tested** infrastructure layer that enables rapid development of high-quality Qt CS-architecture applications — Client on Windows/macOS/Linux desktop, Server on Linux (Ubuntu 20.04+) or cloud — while maintaining strict architectural integrity and developer experience.

## Core Values

### Native First
- Embrace Qt's official recommended practices
- Avoid reimplementing Qt's mature capabilities
- Leverage Qt's native performance and platform integration

### Composition over Inheritance
- Prefer composition patterns over deep inheritance hierarchies
- Promote flexibility and avoid tight coupling

### Interface First
- Define public capabilities through interfaces before implementing
- Enable dependency injection and testability
- Allow multiple implementations for the same interface

### Minimal Dependency
- Maintain minimal dependencies between modules
- Prohibit circular dependencies
- Keep the library lightweight and focused

### Evolution over Prediction
- Follow the "Two-Time Rule": only add features validated by at least two projects
- Avoid over-engineering and speculative design
- Prioritize proven solutions over future predictions

## Positioning

| Aspect | SoulCoreKit | Qt |
|--------|-------------|-----|
| **Scope** | CS Architecture Application Infrastructure | Application Framework |
| **Focus** | Cross-project CS utilities and patterns | Core platform capabilities |
| **Philosophy** | Evolution-driven, minimal, SpringBoot-style scaffold | Comprehensive, feature-rich |
| **Relationship** | Complementary | Foundation |

## Architecture

SoulCoreKit 采用三层架构模型：

```
                     SoulCoreKit
                          │
           ┌──────────────┴──────────────┐
           │                             │
      Foundation                    Application
           │                             │
           │                        ┌────┴────┐
           │                        │         │
       基础设施层                    CS        Web
           │                        │         │
           │                        │    QtWebEngine
           │                        │    (预留)
           │                 Module/Router
           │                 Controller
           │                 Service
           │                 ViewModel
           │                 ErrorHandler
```

- **Foundation 层**: 基础设施能力（core, base, logging, configuration, network, storage, async, event, utils）— 与 UI 无关，与业务无关
- **Application 层**: 业务架构能力（CsModule, CsRouter, CsController, CsService, CsViewModel, CsErrorHandler）
- **CS 模块**: Qt Widgets 桌面客户端，使用 Signal/Slot 驱动
- **Web 模块**: 预留，未来通过 Web Adapter 复用 CS 业务层

详细架构见 [v2.5.0-cs-architecture.md](./v2.5.0-cs-architecture.md)。

## Target Use Cases

- **CS Desktop Application Development**: Build Qt desktop clients with Linux server backends
- **Cross-project Code Reuse**: Share validated components across CS projects
- **Rapid Prototyping**: Start with a solid foundation, focus on business logic
- **Enterprise-grade Applications**: Production-ready components with testing and documentation

## Long-term Vision

SoulCoreKit aims to evolve into a family of specialized libraries:

| Library | Focus |
|---------|-------|
| **SoulCoreKit** | Core infrastructure (current scope, CS shared) |
| **SoulUI** | UI components, animations, themes (Client only) |
| **SoulAI** | AI inference capabilities |
| **SoulRPC** | RPC framework (CS communication) |
| **SoulLSP** | LSP protocol support |
| **SoulMedia** | Audio/video processing |
| **SoulPlugin** | Plugin system |

## Success Criteria

- **API Stability**: Public APIs remain compatible across minor versions
- **Test Coverage**: Core modules achieve >90% unit test coverage
- **Platform Support**: Client on Windows/macOS/Linux; Server on Linux (Ubuntu 20.04+) / cloud
- **Documentation**: Complete API reference and usage guides
- **Community**: Open to external contributions with clear guidelines
- **Performance**: Minimal startup overhead, smooth animations at 60fps