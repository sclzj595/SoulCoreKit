# SoulCoreKit Memory Management Specification

## Overview

This document defines the memory management strategy for SoulCoreKit, including ownership rules, smart pointer usage, and lifecycle management.

---

## Ownership Rules

### QObject Hierarchy

**Rule**: QObject-derived classes use Qt's parent-child hierarchy for ownership.

```cpp
// Parent manages child
QWidget* parent = new QWidget();
QPushButton* button = new QPushButton(parent); // parent owns button

// When parent is destroyed, button is automatically destroyed
delete parent;
```

### Non-QObject Objects

**Rule**: Use `std::unique_ptr` for exclusive ownership.

```cpp
class Service {
private:
    std::unique_ptr<NetworkClient> m_networkClient;
};
```

**Rule**: Use `std::shared_ptr` for shared ownership.

```cpp
std::shared_ptr<Config> config = std::make_shared<Config>();
Service1 s1(config);
Service2 s2(config);
```

### Prohibited Practices

**Prohibited**: Raw `new`/`delete` in business code.

```cpp
// Prohibited
Widget* widget = new Widget(); // Who owns this?
delete widget; // Manual delete error-prone
```

**Correct**:
```cpp
// Use smart pointer or Qt parent
std::unique_ptr<Widget> widget = std::make_unique<Widget>();
```

---

## Smart Pointer Usage

### std::unique_ptr

- **Use Case**: Exclusive ownership, single owner
- **Transfer**: `std::move()`
- **Default**: Prefer `std::unique_ptr` over `std::shared_ptr`

```cpp
std::unique_ptr<HttpClient> client = std::make_unique<HttpClient>();
std::unique_ptr<HttpClient> transferred = std::move(client);
```

### std::shared_ptr

- **Use Case**: Shared ownership, multiple owners
- **Reference Count**: Automatically managed
- **Caution**: Avoid circular references

```cpp
std::shared_ptr<EventBus> bus = std::make_shared<EventBus>();
```

### std::weak_ptr

- **Use Case**: Break circular references
- **Check**: Always check `lock()` returns valid pointer

```cpp
std::shared_ptr<Parent> parent = std::make_shared<Parent>();
parent->child = std::make_shared<Child>();
parent->child->parent = std::weak_ptr<Parent>(parent); // Break cycle
```

---

## Lifecycle Management

### Object Lifecycle

```
Create → Initialize → Running → Destroy → Released
    ↑                           ↓
    └───────────────────────────┘
```

### Manager Lifecycle

```cpp
class BaseManager {
public:
    virtual void init() = 0;
    virtual void shutdown() = 0;
protected:
    bool m_initialized = false;
};
```

**Usage**:
```cpp
auto manager = std::make_unique<NetworkManager>();
manager->init();      // Initialize resources
// ... use ...
manager->shutdown();  // Cleanup resources
```

### Plugin Lifecycle

```cpp
class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual Result<void> load() = 0;
    virtual Result<void> unload() = 0;
};
```

### Event Subscription Lifecycle

```cpp
class Subscription {
public:
    virtual ~Subscription() = default;
    virtual void dispose() = 0;
    virtual bool isActive() const = 0;
};

// Usage
auto subscription = eventBus->subscribe("event", handler);
// ... use ...
subscription->dispose(); // Unsubscribe
```

---

## Singleton Management

### Controlled Lifecycle

Singletons must implement `init()` and `shutdown()`:

```cpp
class Theme : public Singleton<Theme> {
    friend class Singleton<Theme>;
public:
    void init();
    void shutdown();
private:
    Theme() = default;
    ~Theme() = default;
};
```

### Initialization Order

```
Core → Logging → Configuration → Storage → Network → UI
```

### Destruction Order

```
UI → Network → Storage → Configuration → Logging → Core
```

---

## Memory Best Practices

### RAII

- Resource Acquisition Is Initialization
- Use destructors for cleanup
- Prefer stack allocation where possible

```cpp
class FileHandler {
public:
    FileHandler(const QString& path) : m_file(path) {
        m_file.open(QIODevice::ReadOnly);
    }
    ~FileHandler() {
        m_file.close(); // Automatically closed
    }
private:
    QFile m_file;
};
```

### Avoid Memory Leaks

- Always pair `new` with `delete` (or use smart pointers)
- Use Qt's parent-child for QObjects
- Check for dangling pointers

### Avoid Dangling References

- Use `std::weak_ptr` for references that may become invalid
- Check before accessing

```cpp
std::weak_ptr<Data> weakData = strongData;

if (auto data = weakData.lock()) {
    data->process();
}
```

### Memory Debugging

- Enable Qt memory debugging
- Use sanitizers in development
- Run valgrind/memcheck on Linux

---

## Common Patterns

### PImpl Pattern

```cpp
// header
class Widget {
public:
    Widget();
    ~Widget();
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// source
class Widget::Impl {
public:
    QString text;
    QColor color;
};

Widget::Widget() : m_impl(std::make_unique<Impl>()) {}
Widget::~Widget() = default;
```

### Factory Pattern

```cpp
class Factory {
public:
    std::unique_ptr<INetworkClient> createClient() {
        return std::make_unique<DefaultNetworkClient>();
    }
};
```

---

## Memory Management Review Checklist

- ☑ QObjects use parent-child ownership
- ☑ Non-QObjects use smart pointers
- ☑ No raw `new`/`delete` in business code
- ☑ `std::unique_ptr` for exclusive ownership
- ☑ `std::shared_ptr` for shared ownership
- ☑ `std::weak_ptr` to break circular references
- ☑ Managers implement `init()`/`shutdown()`
- ☑ Singletons have controlled lifecycle
- ☑ RAII used for resource management
- ☑ No dangling pointers or references