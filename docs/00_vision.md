# SoulCoreKit Vision

## Project Overview

SoulCoreKit is a lightweight, modular, and evolution-driven C++/Qt infrastructure library designed for **Client-Server (CS) architecture** application development. It serves as the foundation for CS-style projects (IDE, Music Player, Radio, Notebook, IM, EatDecider, etc.) by providing consistent, production-grade components and utilities. The scaffold design borrows from SpringBoot's IoC/DI/AOP/lifecycle concepts, but the architecture is CS (Qt desktop client + Linux server), not BS (Browser-Server).

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

SoulCoreKit is a **CS (Client-Server) architecture** scaffold, not a BS (Browser-Server) framework:

- **Client**: Qt desktop application on Windows/macOS/Linux, using SoulCoreKit + SoulCoreKitUi
- **Server**: Linux (Ubuntu 20.04+) or cloud server backend process, using SoulCoreKit (without UI)
- **Communication**: HTTP/TCP/WebSocket/RPC between Client and Server
- **Scaffold style**: Borrows SpringBoot's IoC/DI/AOP/module lifecycle concepts, but does NOT replicate Servlet/Tomcat/Filter/MVC etc.

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