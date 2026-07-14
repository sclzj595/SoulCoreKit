# SoulCoreKit Architecture Specification

## Overview

SoulCoreKit follows a layered, modular architecture with strict dependency rules. Each module has a clear responsibility and well-defined interfaces.

## Layered Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                        UI Layer                             │
│  Widgets, Themes, Animations, Dialogs, Navigation          │
├─────────────────────────────────────────────────────────────┤
│                      Business Layer                         │
│  Auth, Service, Repository, ViewModel                      │
├─────────────────────────────────────────────────────────────┤
│                      Infrastructure Layer                  │
│  Network, Storage, Event, Configuration, Logging           │
├─────────────────────────────────────────────────────────────┤
│                      Foundation Layer                       │
│  Core (Result, Error, Interface), Base (BaseObject, etc.)  │
└─────────────────────────────────────────────────────────────┘
```

## Module Structure

```
SoulCoreKit/
├── Core              # Foundation (Qt Core only)
├── Base              # Base classes (extends Core)
├── Logging           # Logging system
├── Configuration     # Configuration management
├── Network           # Network communication
├── Storage           # Data persistence
├── Async             # Asynchronous tasks
├── Event             # Event bus
├── UI                # UI components
├── Auth              # Authentication
├── Utils             # Utility functions
├── Examples          # Demo applications
├── Tests             # Unit tests
└── Docs              # Documentation
```

## Module Dependency Graph

### Allowed Dependencies

```
UI ────────→ Core
 │            ↑
 └──────→ Base ─┘

Auth ───────→ Core
  │            ↑
  ├──→ Network ─┤
  ├──→ Storage ─┤
  └──→ Utils ────┘

Network ────→ Core
  │            ↑
  └──→ Logging ─┘

Storage ─────→ Core

Configuration → Core
  │            ↑
  └──→ Utils ──┘

Event ───────→ Core

Async ───────→ Core
  │            ↑
  └──→ Logging ─┘

Logging ─────→ Core

Utils ───────→ Core

Base ────────→ Core
```

### Prohibited Dependencies

- **Core** must NOT depend on any other module
- **Infrastructure modules** (Network, Storage, Event, etc.) must NOT depend on UI
- **No circular dependencies** allowed between any modules

## Module Responsibility Matrix

| Module | Responsibility | Dependencies | Thread Safety |
|--------|---------------|--------------|---------------|
| **Core** | Foundation types (Result, Error, Interface, Singleton) | Qt Core | Yes |
| **Base** | Base classes (BaseObject, BaseManager, etc.) | Core, UI | Yes |
| **Logging** | Logging system with sink architecture | Core | Yes |
| **Configuration** | Configuration management, hot-reload | Core, Utils | Yes |
| **Network** | HTTP, WebSocket, TCP, download/upload | Core, Logging | Yes |
| **Storage** | SQLite, Cache, Settings, Serialization | Core | Yes |
| **Async** | ThreadPool, Task, Dispatcher | Core, Logging | Yes |
| **Event** | Event bus, subscription management | Core | Yes |
| **UI** | Widgets, Themes, Animations, Dialogs | Core, Base | GUI Thread only |
| **Auth** | Authentication, Token, Permission | Core, Network, Storage | Yes |
| **Utils** | Helper utilities (json, file, string, etc.) | Core | Yes |

## Component Interactions

### Application Lifecycle

```
Application::start()
    │
    ├── Core::initialize()
    ├── Logging::initialize()
    ├── Configuration::load()
    ├── EventBus::create()
    ├── Storage::initialize()
    ├── Network::initialize()
    ├── UI::initialize()
    │
    └── Application::run()
            │
            └── Application::shutdown()
                    ├── UI::shutdown()
                    ├── Network::shutdown()
                    ├── Storage::shutdown()
                    ├── EventBus::destroy()
                    ├── Configuration::save()
                    ├── Logging::shutdown()
                    └── Core::shutdown()
```

### Dependency Injection Pattern

Services should be injected via constructor rather than accessed globally:

```cpp
// Recommended
class UserService {
public:
    UserService(INetworkClient* network, IStorage* storage)
        : m_network(network), m_storage(storage) {}
private:
    INetworkClient* m_network;
    IStorage* m_storage;
};

// Prohibited
class UserService {
public:
    UserService() {
        m_network = NetworkClient::instance();  // Global access
    }
};
```

## Key Design Patterns

### 1. Interface-Based Design

All public services must expose interfaces:

```cpp
class INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual Result<HttpResponse> get(const QString& url) = 0;
    virtual Result<HttpResponse> post(const QString& url, const QByteArray& body) = 0;
};
```

### 2. Singleton with Controlled Lifecycle

Singletons must implement `init()` and `shutdown()`:

```cpp
class Theme : public Singleton<Theme> {
    friend class Singleton<Theme>;
public:
    void init();
    void shutdown();
private:
    Theme() = default;
};
```

### 3. Factory Pattern

Use factories for creating objects by name:

```cpp
class IFactory {
public:
    virtual QObject* create(const QString& type) = 0;
};
```

### 4. Observer Pattern

Event bus implements observer for decoupled communication:

```cpp
class IEventBus {
public:
    virtual Subscription subscribe(const QString& eventType, Callback callback) = 0;
    virtual void publish(const QString& eventType, const QVariant& data) = 0;
};
```

## Data Flow

### Request-Response Flow

```
UI Layer                          Infrastructure Layer
   │                                     │
   │── UserAction ──→ ViewModel          │
   │         │                           │
   │         └── Service::execute()      │
   │                     │               │
   │                     ↓               │
   │              Network::request() ────┼──→ HTTP Request
   │                     │               │
   │                     ↓               │
   │              Network::response() ←──┼──← HTTP Response
   │                     │               │
   │                     ↓               │
   │              Service::handleResult()│
   │                     │               │
   │                     ↓               │
   │         ViewModel::updateState()    │
   │                     │               │
   │                     ↓               │
   │── UI::update() ←───┘               │
```

### Event-Driven Flow

```
Producer                          EventBus                          Consumer
   │                                 │                                 │
   │── publish(event) ───────────────→│                                 │
   │                                 │── dispatch(event) ──────────────→│
   │                                 │                                 │── handle(event)
```

## Architecture Constraints

1. **No UI in Infrastructure**: Network, Storage, Event modules must not contain any UI code
2. **No Business Logic in UI**: UI components must only handle presentation, not business logic
3. **Interface Segregation**: Interfaces should be small and focused
4. **Dependency Inversion**: Depend on abstractions, not concretions
5. **Single Responsibility**: Each class should have only one reason to change

## Anti-Patterns to Avoid

- **God Objects**: Classes that do too much (e.g., a service that also handles UI)
- **Global State**: Avoid singleton abuse; prefer dependency injection
- **Circular Dependencies**: Use interfaces to break cycles
- **Deep Inheritance**: Prefer composition over inheritance
- **Magic Strings**: Use enums or constants instead of raw strings