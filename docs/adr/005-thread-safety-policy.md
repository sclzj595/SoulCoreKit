# ADR-005: Thread Safety Policy

| | |
|---|---|
| **Status** | Accepted |
| **Date** | 2026-07-25 |
| **Module** | Core, Async, Data, Event, DI |

## Context

SoulCoreKit supports multi-threaded operation (thread pools, async I/O, event dispatch, background workers). Without a consistent thread safety policy:

- Race conditions corrupt shared state (connection pools, caches, event registrations)
- Deadlocks occur from inconsistent lock ordering
- Singleton initialization suffers from double-checked locking bugs
- Lock contention becomes a bottleneck in high-QPS scenarios

## Decision

### Classification of Thread Safety Levels

Every class in SoulCoreKit is classified into one of four thread-safety levels. The classification is documented in the class header and enforced by design.

| Level | Guarantee | Implementation | Use Case |
|-------|-----------|---------------|----------|
| **Immutable** | No mutable state | None needed | Value objects, config snapshots |
| **Thread-Safe** | Safe for concurrent access | Internal synchronization | Singletons, DI containers, connection pools |
| **Conditional** | Safe only with external synchronization | Documented lock requirements | Repositories (require external transaction scope) |
| **Single-Threaded** | Must not be shared between threads | Qt thread affinity, `Q_ASSERT(thread() == ...)` | QObject UI widgets, Qt signal/slot objects |

### Synchronization Primitives

#### 1. Lock-Based Synchronization

Use for complex shared state where lock-free algorithms are impractical:

```cpp
// Mutex — simple mutual exclusion
class ConnectionPool {
    mutable std::mutex m_mutex;
    std::vector<std::unique_ptr<IDatabaseDriver>> m_connections;

    Result<std::unique_ptr<IDatabaseDriver>> acquire() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_connections.empty()) {
            auto conn = std::move(m_connections.back());
            m_connections.pop_back();
            return std::move(conn);
        }
        // create new...
    }
};

// Recursive Mutex — for DI container (re-entrant resolution)
class Container {
    mutable std::recursive_mutex m_mutex;
    // resolve<T>() may call bind() → lock_guard → re-entrant lock acquisition
};
```

#### 2. Lock-Free / Atomic Synchronization

Use for simple counters, flags, and pointer swaps where lock-free is feasible:

```cpp
// Atomic counter for active tasks
class TaskRunner : public QObject {
    std::atomic<int> m_activeTasks{0};

    Future<ResultType> run(F&& func) {
        m_activeTasks.fetch_add(1, std::memory_order_relaxed);
        QThreadPool::globalInstance()->start([this, task, promise]() {
            // ... execute task ...
            m_activeTasks.fetch_sub(1, std::memory_order_release);
        });
    }
};

// Atomic flag for singleton initialization
template<typename T>
class SharedSingleton {
    static std::shared_ptr<T> m_instance;
    static std::mutex m_mutex;
    static std::atomic<bool> m_initialized;

    static void init() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_instance && !m_initialized) {
            m_instance->init();
            m_initialized.store(true, std::memory_order_release);
        }
    }
};
```

#### 3. Lock-Free Data Structures

Use for high-contention scenarios where locks become bottlenecks:

```cpp
// RCU (Read-Copy-Update) pattern for event bus
class EventBus {
    // Read path: lock-free (RCU)
    template<typename T>
    void publish(const T& event) {
        auto handlers = m_handlers.load(std::memory_order_acquire);
        for (auto& handler : *handlers) {
            handler(event);
        }
    }

    // Write path: lock + copy-on-write
    template<typename T>
    Subscription subscribe(Handler<T> handler) {
        std::lock_guard<std::mutex> lock(m_updateMutex);
        auto oldHandlers = m_handlers.load();
        auto newHandlers = std::make_shared<HandlerList>(*oldHandlers);
        newHandlers->push_back(std::make_shared<Handler<T>>(std::move(handler)));
        m_handlers.store(newHandlers, std::memory_order_release);
        // oldHandlers is implicitly released when last reader finishes
    }

private:
    std::shared_ptr<HandlerList> m_handlers;  // atomic via shared_ptr
    std::mutex m_updateMutex;                  // write-only mutex
};
```

### Lock Ordering

To prevent deadlocks, all locks in SoulCoreKit must follow this global order:

```
1. m_mutex (application-level / container-level)
2. m_connectionMutex (connection pools)
3. m_cacheMutex (caches)
4. m_eventMutex (event buses)
5. m_threadMutex (thread pools)
6. m_dataMutex (data repositories)
```

When acquiring multiple locks, always acquire them in ascending order. Violating this order causes deadlocks and is explicitly banned.

### Singleton Thread Safety

```cpp
// Pattern: Meyers' Singleton (C++11 guaranteed thread-safe)
template<typename T>
class Singleton {
public:
    static T& instance() {
        static T inst;  // C++11: thread-safe initialization
        return inst;
    }
};

// Pattern: SharedSingleton with explicit init/destroy
template<typename T>
class SharedSingleton {
    static std::shared_ptr<T> instance() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_instance) {
            m_instance = std::make_shared<T>();
            m_initialized = false;
        }
        return m_instance;
    }

    static void init() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_instance && !m_initialized) {
            m_instance->init();
            m_initialized = true;  // publish with release semantics
        }
    }
};
```

### Qt Thread Affinity

QObject subclasses with Qt signals/slots must respect Qt's thread affinity:

```cpp
class NetworkClient : public QObject {
    Q_OBJECT
public:
    // Must be created and used on the same thread
    // OR moved to a specific thread with moveToThread()
    explicit NetworkClient(QObject* parent = nullptr);

signals:
    void dataReceived(const QByteArray& data);
    void errorOccurred(const QString& error);

private slots:
    void onReadyRead();  // Always executed on the object's thread
};
```

## Consequences

### Positive
- **Deadlock prevention**: Enforced lock ordering eliminates the most common source of deadlocks
- **High throughput**: Lock-free patterns for hot paths (event dispatch, task counters) minimize contention
- **Clear documentation**: Every class's thread-safety level is part of its public contract
- **Correct singletons**: Meyers' Singleton pattern leverages C++11 guarantees
- **Qt integration**: Clear boundary between Qt thread-affine objects and thread-safe framework objects

### Negative
- **Complexity**: RCU patterns and lock-free data structures are harder to reason about
- **Memory overhead**: RCU read-side copies can increase memory usage during updates
- **Lock contention**: Some locks (e.g., DI container's recursive_mutex) may become bottlenecks under extreme contention
- **Qt constraint**: QObject subclasses must not be shared between threads without `moveToThread()`

## Enforcement

1. **Code review**: Every new mutable shared state must be classified and synchronized per this policy
2. **Thread-safety documentation**: Class-level thread-safety classification must appear in the class doc block
3. **Static analysis** (future): Clang-Tidy check for lock ordering violations
4. **Testing**: ThreadSanitizer (TSan) runs on all multi-threaded unit tests in CI
5. **Benchmarking**: Lock-free vs. lock-based performance is benchmarked for all hot paths

## Thread Safety Classification Diagram

```mermaid
flowchart TB
    subgraph "Classification"
        direction LR
        I["Immutable<br/>No sync needed"]
        T["Thread-Safe<br/>Internal sync"]
        C["Conditional<br/>External sync"]
        S["Single-Threaded<br/>Qt affinity"]
    end

    subgraph "Synchronization Primitives"
        direction TB
        LOCK["Lock-Based<br/>mutex / recursive_mutex"]
        ATOM["Atomic / Lock-Free<br/>atomic<T>, RCU"]
    end

    subgraph "Lock Order"
        direction TB
        LO1["1. Container / App mutex"]
        LO2["2. Connection pool mutex"]
        LO3["3. Cache mutex"]
        LO4["4. Event bus mutex"]
        LO5["5. Thread pool mutex"]
        LO6["6. Repository mutex"]
    end

    T --> LOCK
    T --> ATOM
    C --> LOCK
    S --> LOCK

    style I fill:#c8e6c9,color:#1a5e20
    style T fill:#bbdefb,color:#0d47a1
    style C fill:#fff3e0,color:#e65100
    style S fill:#f3e5f5,color:#7b1fa2
    style LOCK fill:#ffebee,color:#b71c1c
    style ATOM fill:#e8f5e9,color:#1b5e20