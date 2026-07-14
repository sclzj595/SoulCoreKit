# SoulCoreKit Vision

## Project Overview

SoulCoreKit is a lightweight, modular, and evolution-driven C++/Qt infrastructure library designed for desktop application development. It serves as the foundation for all desktop projects (IDE, Music Player, Radio, Notebook, IM, EatDecider, etc.) by providing consistent, production-grade components and utilities.

## Mission

To provide a **stable, maintainable, and battle-tested** infrastructure layer that enables rapid development of high-quality Qt desktop applications while maintaining strict architectural integrity and developer experience.

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
| **Scope** | Application Infrastructure | Application Framework |
| **Focus** | Cross-project utilities and patterns | Core platform capabilities |
| **Philosophy** | Evolution-driven, minimal | Comprehensive, feature-rich |
| **Relationship** | Complementary | Foundation |

## Target Use Cases

- **Desktop Application Development**: Build modern, native-feeling desktop apps
- **Cross-project Code Reuse**: Share validated components across projects
- **Rapid Prototyping**: Start with a solid foundation, focus on business logic
- **Enterprise-grade Applications**: Production-ready components with testing and documentation

## Long-term Vision

SoulCoreKit aims to evolve into a family of specialized libraries:

| Library | Focus |
|---------|-------|
| **SoulCoreKit** | Core infrastructure (current scope) |
| **SoulUI** | UI components, animations, themes |
| **SoulAI** | AI inference capabilities |
| **SoulRPC** | RPC framework |
| **SoulLSP** | LSP protocol support |
| **SoulMedia** | Audio/video processing |
| **SoulPlugin** | Plugin system |

## Success Criteria

- **API Stability**: Public APIs remain compatible across minor versions
- **Test Coverage**: Core modules achieve >90% unit test coverage
- **Platform Support**: Windows, macOS, Linux with consistent behavior
- **Documentation**: Complete API reference and usage guides
- **Community**: Open to external contributions with clear guidelines
- **Performance**: Minimal startup overhead, smooth animations at 60fps