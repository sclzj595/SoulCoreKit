# ADR-002: Module Dependency Rules

| | |
|---|---|
| **Status** | Accepted |
| **Date** | 2026-07-25 |
| **Module** | All modules |

## Context

SoulCoreKit is organized into horizontal layers (L0 through L4) and feature modules (core, logging, data, async, di, orm, network, ui, etc.). Without explicit dependency rules, the project risks:

- Circular dependencies between modules, leading to build failures and undefined initialization order
- Downward dependencies where low-level modules depend on high-level ones (e.g., `core` includes `ui`)
- Hidden coupling that makes independent testing and reuse impossible
- Large rebuild chains when a leaf module changes

## Decision

### Layer Model

SoulCoreKit is organized into four layers. Dependencies flow **only upward** (lower layers may not depend on higher layers):

```
┌─────────────────────────────────────────┐
│  L4  UI, Plugin, RPC, MQ, Network        │  ← Application-facing features
├─────────────────────────────────────────┤
│  L3  ORM, Storage, Auth, Configuration  │  ← Domain services
├─────────────────────────────────────────┤
│  L2  Data, Event, Async, DI             │  ← Infrastructure services
├─────────────────────────────────────────┤
│  L1  Logging, Utils                      │  ← Cross-cutting concerns
├─────────────────────────────────────────┤
│  L0  Core (Result, Error, Singleton,     │  ← Foundation — depends on nothing
│       Module, Interface, Time, UUID)     │
└─────────────────────────────────────────┘
```

### Rules

1. **No circular dependencies**: Module A cannot include Module B if Module B already includes Module A (transitively)
2. **No downward dependencies**: L0 modules must never include headers from L1–L4
3. **Implicit allowed dependencies** (every module may depend on L0):
   - All modules → `core` (Result, Error, Singleton, Interface, Time, UUID, Module)
4. **Explicit allowed cross-module dependencies**:

| Source | May depend on | Rationale |
|--------|--------------|-----------|
| `core` | — | Foundation layer, depends on nothing |
| `logging` | `core` | Needs Result/Error for sink reporting |
| `data` | `core`, `logging` | Repository pattern uses Result; logs errors |
| `async` | `core`, `logging` | Future/Promise/TaskRunner use Result; logs exceptions |
| `di` | `core` | Container uses Result; Singleton pattern |
| `event` | `core`, `async`, `logging` | EventBus uses ThreadPool; logs handler errors |
| `orm` | `core`, `data`, `logging` | Repository base uses IDatabaseDriver; logs query errors |
| `storage` | `core`, `data`, `logging` | Storage backends use Repository; logs I/O errors |
| `auth` | `core`, `data`, `logging` | Token storage uses Repository; logs auth failures |
| `network` | `core`, `async`, `logging`, `di` | HTTP client uses async; logs network errors; resolves adapters |
| `ui` | `core`, `async`, `event`, `logging` | Widgets use async operations; subscribes to events; logs rendering errors |
| `plugin` | `core`, `di`, `logging` | Plugin resolution uses DI; logs load failures |
| `rpc` | `core`, `async`, `di`, `network` | Dispatcher uses async; resolves transports via DI |
| `mq` | `core`, `async`, `logging` | Message producers/consumers use async; logs MQ errors |

### Forbidden Dependencies

- `core` → `ui`, `network`, `orm`, `plugin`, `rpc`, `mq` (L0 must not know about L4 features)
- `logging` → anything beyond `core` (sinks must be lightweight)
- `data` → `ui`, `network`, `plugin` (repository must be UI-agnostic)
- `async` → `ui` (thread pool must not depend on Qt GUI)
- Any → `ui` except explicitly approved presentation-layer modules
- Any circular dependency, direct or transitive

## Consequences

### Positive
- **Independent build**: Lower layers can be compiled and tested without higher layers
- **Clear initialization order**: L0 initializes first, then L1, L2, L3, L4
- **Reusability**: Core and infrastructure modules can be reused in non-Qt console applications
- **Minimal rebuilds**: Changing a leaf module (e.g., `ui/button.cpp`) does not trigger core rebuilds
- **Testability**: L0–L2 modules can be unit-tested without a QApplication instance

### Negative
- **Occasional cross-cutting tension**: Some concerns (e.g., configuration) want to be both infrastructure and feature-level; the layer model forces a choice
- **Interface bloat**: To avoid upward dependencies, some modules may expose more interfaces than ideal
- **Requires vigilance**: Without tooling, developers may accidentally introduce upward dependencies

## Enforcement

1. **Include order in headers**: Headers should include only the minimum necessary. Use forward declarations for types used only as pointers/references.
2. **Code review**: Every PR touching `#include` directives must be reviewed for dependency compliance.
3. **Future tooling**: A CMake script or Clang-Tidy plugin can generate a dependency graph and flag violations.
4. **Build system**: CMake target-linking dependencies mirror the layer model; attempting to link a lower layer against a higher layer fails at link time.

## Dependency Flow Diagram

```mermaid
flowchart TB
    subgraph L4["L4 — Application Features"]
        UI[ui]
        Network[network]
        Plugin[plugin]
        RPC[rpc]
        MQ[mq]
    end

    subgraph L3["L3 — Domain Services"]
        ORM[orm]
        Storage[storage]
        Auth[auth]
        Config[configuration]
    end

    subgraph L2["L2 — Infrastructure"]
        Data[data]
        Event[event]
        Async[async]
        DI[di]
    end

    subgraph L1["L1 — Cross-Cutting"]
        Logging[logging]
        Utils[utils]
    end

    subgraph L0["L0 — Foundation"]
        Core[core]
    end

    L4 --> L3
    L4 --> L2
    L4 --> L1
    L4 --> L0
    L3 --> L2
    L3 --> L1
    L3 --> L0
    L2 --> L1
    L2 --> L0
    L1 --> L0

    Core --> Core
    Logging --> Core
    Data --> Core
    Data --> Logging
    Async --> Core
    Async --> Logging
    DI --> Core
    Event --> Core
    Event --> Async
    Event --> Logging
    ORM --> Core
    ORM --> Data
    ORM --> Logging

    style L0 fill:#c8e6c9,color:#1a5e20
    style L1 fill:#bbdefb,color:#0d47a1
    style L2 fill:#fff3e0,color:#e65100
    style L3 fill:#f3e5f5,color:#7b1fa2
    style L4 fill:#ffebee,color:#b71c1c