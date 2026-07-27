# ADR-003: Memory Management Policy

| | |
|---|---|
| **Status** | Accepted |
| **Date** | 2026-07-25 |
| **Module** | All modules |

## Context

SoulCoreKit is a C++/Qt framework where memory mismanagement is the highest-risk source of bugs. Without a unified memory policy:

- Double-free and use-after-free bugs become common when ownership semantics are unclear
- Qt's parent-child ownership model (raw `new` with `this` parent) conflicts with modern C++ RAII practices
- Memory leaks accumulate in long-lived services (connection pools, thread pools, event buses)
- Cyclic references between `shared_ptr` objects cause unbounded memory growth

## Decision

### Policy Matrix

| Allocation Pattern | Mechanism | Deleter | Use Case |
|--------------------|-----------|---------|----------|
| **Default** | `std::unique_ptr<T>` | Automatic (RAII) | All heap allocations with single ownership |
| **Shared ownership** | `std::shared_ptr<T>` | Automatic (reference count) | DI containers, event buses, cached data |
| **Qt parent-child** | Raw `new` with Qt parent | Qt's `deleteLater` | QObject subclasses where parent guarantees cleanup |
| **QScopedPointer** | `QScopedPointer<T>` (deprecated) or `std::unique_ptr` with custom deleter | Manual or custom | QObject subclasses with explicit ownership transfer |
| **Placement / internal** | `std::pmr::polymorphic_allocator` or custom pool | Custom | Performance-critical allocations (future) |

### Rules

#### Rule 1: No raw `new`/`delete` in application code, except:

**Allowed — Qt parent-child ownership:**
```cpp
// Widget with parent — Qt guarantees deletion when parent is destroyed
m_layout = new QVBoxLayout(this);          // QLayoutItem cleanup by Qt
m_socket = new QTcpSocket(this);            // QObject with parent
m_manager = new QNetworkAccessManager(this); // QObject with parent
m_animation = new QPropertyAnimation(this, "geometry"); // QObject with parent
```

These are allowed because:
- The `this` parameter establishes Qt's parent-child ownership
- When the parent QObject is destroyed, Qt automatically deletes all children
- The raw `new` is idiomatic Qt and provides no benefit from `unique_ptr`

**Allowed — QLayoutItem cleanup:**
```cpp
// Removing a widget from a layout requires manual cleanup
delete item->widget();   // QWidget not owned by layout after removal
delete item;             // QLayoutItem cleanup
```

**Allowed — Type-erased deleters in DI container:**
```cpp
// di/container.h — by design, the container owns deletion
info.deleter = [](void* ptr) { delete static_cast<T*>(ptr); };
```

This is the single sanctioned use of raw `delete` in a header, confined to the DI container's type-erased deleter mechanism.

#### Rule 2: All other heap allocations use smart pointers

```cpp
// CORRECT — unique_ptr for single ownership
auto conn = std::make_unique<QTcpSocket>();
auto task = std::make_shared<CancelableTask>(func);
auto dial = std::unique_ptr<ISqlDialect>(ISqlDialect::create(type));

// CORRECT — shared_ptr for shared ownership
auto pool = std::make_shared<DefaultDbConnectionPool>(config);
auto bus = TypedEventBus::create();  // returns shared_ptr
```

#### Rule 3: `delete` keyword is forbidden in application code

The `delete` keyword may only appear in:
- Qt parent-child cleanup (QLayoutItem, as shown above)
- The DI container's type-erased deleter (`di/container.h`)
- Custom deleters passed to `shared_ptr` / `unique_ptr` constructors

#### Rule 4: QScopedPointer migration

`QScopedPointer` is deprecated in Qt 6. New code must use `std::unique_ptr` with appropriate custom deleters for QObject subclasses:

```cpp
// If you need unique ownership of a QObject without parent:
struct QObjectDeleter {
    void operator()(QObject* obj) const {
        if (obj) obj->deleteLater();
    }
};
using QObjectPtr = std::unique_ptr<QObject, QObjectDeleter>;
```

#### Rule 5: Shared pointer cycle prevention

When `shared_ptr` cycles are possible, use `std::weak_ptr` to break them:

```cpp
class Subscription {
    std::weak_ptr<TypedEventBus> m_bus;  // prevents cycle
public:
    void unsubscribe() {
        if (auto bus = m_bus.lock()) {
            bus->removeHandler(m_key);
        }
    }
};
```

## Consequences

### Positive
- **Exception safety**: RAII guarantees cleanup even when exceptions propagate
- **Ownership clarity**: Smart pointer types document ownership semantics at the type level
- **Zero overhead**: `unique_ptr` has zero runtime cost compared to raw pointers
- **Qt compatibility**: The policy recognizes Qt's parent-child model as a valid RAII pattern
- **Searchable**: A grep for `delete` finds exactly the sanctioned locations

### Negative
- **Learning curve**: Team members must understand Qt's parent-child semantics vs. C++ smart pointers
- **Qt parent traps**: Forgetting to pass a parent to `new QObject()` creates a leak; passing the wrong parent causes double-free
- **Shared_ptr overhead**: Atomic reference counting adds overhead in hot paths

## Enforcement

1. **Code review checklist**: Every `new` in source must be justified as a Qt parent-child pattern
2. **Static analysis** (future): Clang-Tidy `cppcoreguidelines-owning-memory` and `cppcoreguidelines-no-malloc` checks
3. **Memory profiling**: CI runs leak detection (Valgrind, AddressSanitizer, Visual Studio Memory Analysis) on all unit tests
4. **Documentation**: `docs/09_memory_management.md` provides a reference with all patterns

## Memory Ownership Diagram

```mermaid
flowchart LR
    subgraph Raw["Raw new/delete (FORBIDDEN except Qt parent)"]
        direction TB
        R1["new QWidget(this)<br/>Qt parent → auto-delete"]
        R2["new QLayoutItem<br/>delete in cleanup"]
    end

    subgraph Smart["Smart Pointers (REQUIRED)"]
        direction TB
        S1["unique_ptr → single ownership"]
        S2["shared_ptr → shared ownership"]
        S3["weak_ptr → cycle breaker"]
    end

    subgraph DI["DI Container (SANCTIONED delete)"]
        direction TB
        D1["Type-erased deleter<br/>delete static_cast T"]
    end

    style Raw fill:#ffcdd2,color:#b71c1c
    style Smart fill:#c8e6c9,color:#1a5e20
    style DI fill:#fff3e0,color:#e65100