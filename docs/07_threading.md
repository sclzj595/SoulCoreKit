# SoulCoreKit Thread Model

## Overview

This document defines the thread model for SoulCoreKit, specifying which components run on which threads and how cross-thread communication should be handled.

---

## Thread Types

### GUI Thread

**Responsibility**:
- All UI operations (QWidget, QPainter, etc.)
- Event processing
- UI component lifecycle management

**Restrictions**:
- Only the GUI thread can create, modify, or destroy QWidget instances
- No blocking operations allowed
- Must remain responsive at all times

### Worker Thread

**Responsibility**:
- CPU-bound tasks (computations, data processing)
- Background operations
- Long-running tasks

**Restrictions**:
- No UI operations
- Use `Dispatcher` to update UI

### Network Thread

**Responsibility**:
- Network I/O (HTTP, WebSocket, TCP)
- Socket operations
- Download/upload tasks

**Restrictions**:
- No UI operations
- Use `Dispatcher` to update UI

### Database Thread

**Responsibility**:
- SQLite operations
- Data persistence
- Query execution

**Restrictions**:
- No UI operations
- Use `Dispatcher` to update UI

### Timer Thread

**Responsibility**:
- Timers
- Periodic tasks

**Restrictions**:
- No UI operations
- Short execution time only

---

## Thread Safety

### Thread-Safe Modules

These modules are designed to be thread-safe and can be accessed from any thread:

- **Core**: `Result`, `Error`, `Uuid`, `Version`, `Time`
- **Logging**: `Logger`, all sinks
- **Configuration**: `Config`, `IConfiguration`
- **Network**: `HttpClient`, `WebSocket`, `TcpClient`
- **Storage**: `SqliteDatabase`, `Settings`, `ICache`
- **Async**: `ThreadPool`, `TaskRunner`, `Dispatcher`
- **Event**: `IEventBus`, `DefaultEventBus`
- **Utils**: All utility functions

### GUI-Thread-Only Modules

These modules must only be accessed from the GUI thread:

- **UI**: All widgets, `Theme`, `Style`, `Animation`
- **Base**: `BaseWidget`, `BaseDialog`, `BaseViewModel`

---

## Cross-Thread Communication

### Dispatcher Pattern

Use `Dispatcher` to safely execute code on the GUI thread:

```cpp
// From worker thread
sc::Dispatcher::invoke([]() {
    // This code runs on GUI thread
    m_label->setText("Result ready");
});

// With parameter
sc::Dispatcher::invoke([result]() {
    m_label->setText(QString::number(result));
});

// Async invocation
sc::Dispatcher::invokeAsync([]() {
    // Non-blocking
});
```

### Signal-Slot Mechanism

Qt's signal-slot mechanism automatically handles cross-thread communication:

```cpp
// Worker class
class Worker : public QObject {
    Q_OBJECT
signals:
    void resultReady(int value);
};

// In GUI thread
Worker* worker = new Worker();
connect(worker, &Worker::resultReady, this, [](int value) {
    m_label->setText(QString::number(value));
});
```

### Event Bus

Event bus supports both synchronous and asynchronous dispatch:

```cpp
// Sync dispatch (same thread)
eventBus->publish("user.logged_in", userData);

// Async dispatch (worker thread → GUI thread)
eventBus->publishAsync("data.loaded", data);
```

---

## Thread Pool

### Global Thread Pool

- Managed by `ThreadPool` singleton
- Automatically sized based on CPU cores
- Default: `QThread::idealThreadCount()` threads

### Task Submission

```cpp
auto future = sc::ThreadPool::instance()->submit([]() {
    return heavyComputation();
});
```

### Thread Affinity

- Tasks are dispatched to available threads
- No guarantee of thread affinity unless explicitly set
- Use `QObject::moveToThread()` for long-running objects

---

## Synchronization

### Mutex Guidelines

- Use `QMutex` for shared resources
- Use `QReadWriteLock` for read-heavy scenarios
- Prefer fine-grained locking over coarse-grained

**Example**:
```cpp
class ThreadSafeCache {
public:
    Result<void> set(const QString& key, const QByteArray& value) {
        QMutexLocker locker(&m_mutex);
        m_cache[key] = value;
        return Ok();
    }
    
    Result<QByteArray> get(const QString& key) {
        QMutexLocker locker(&m_mutex);
        auto it = m_cache.find(key);
        if (it == m_cache.end()) {
            return Err(ErrorCode::NotFound);
        }
        return Ok(it.value());
    }
    
private:
    QMutex m_mutex;
    QHash<QString, QByteArray> m_cache;
};
```

### Avoid Deadlocks

- Always acquire locks in the same order
- Avoid nested locks
- Use `QMutexLocker` for automatic unlock

---

## Prohibited Practices

### Cross-Thread Widget Access

**Prohibited**:
```cpp
// In worker thread
QWidget* widget = new QWidget(); // WRONG
widget->show(); // WRONG
```

**Correct**:
```cpp
// In worker thread
sc::Dispatcher::invoke([]() {
    QWidget* widget = new QWidget();
    widget->show();
});
```

### Blocking GUI Thread

**Prohibited**:
```cpp
// In GUI thread
QThread::sleep(5000); // Blocks UI
```

**Correct**:
```cpp
// Move heavy work to worker thread
auto future = sc::async([]() {
    return heavyOperation();
}).then([](Result<Data> result) {
    // Update UI
});
```

### Shared Mutable State

**Prohibited**:
```cpp
// Shared without synchronization
int g_counter = 0;

// Thread 1
g_counter++;

// Thread 2
g_counter++;
```

**Correct**:
```cpp
// Use atomic or mutex
std::atomic<int> g_counter{0};

// Thread 1
g_counter.fetch_add(1);

// Thread 2
g_counter.fetch_add(1);
```

---

## Thread Model Review Checklist

- ☑ UI operations only in GUI thread
- ☑ Uses `Dispatcher` for cross-thread UI updates
- ☑ Thread-safe modules properly synchronized
- ☑ No blocking operations in GUI thread
- ☑ Uses `QMutexLocker` for automatic unlock
- ☑ Signal-slot connections handle cross-thread
- ☑ Event bus uses async dispatch for GUI updates
- ☑ No global mutable state without synchronization
- ☑ Tasks submitted to thread pool instead of manual threads
- ☑ `QObject::moveToThread()` used for long-running objects