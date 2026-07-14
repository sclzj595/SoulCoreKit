# SoulCoreKit 线程模型

## 概述

本文档定义了 SoulCoreKit 的线程模型，指定哪些组件运行在哪些线程上，以及如何处理跨线程通信。

---

## 线程类型

### GUI 线程

**职责**：
- 所有 UI 操作（QWidget、QPainter 等）
- 事件处理
- UI 组件生命周期管理

**限制**：
- 只有 GUI 线程可以创建、修改或销毁 QWidget 实例
- 不允许阻塞操作
- 必须始终保持响应

### Worker 线程

**职责**：
- CPU 密集型任务（计算、数据处理）
- 后台操作
- 长时间运行的任务

**限制**：
- 无 UI 操作
- 使用 `Dispatcher` 更新 UI

### Network 线程

**职责**：
- 网络 I/O（HTTP、WebSocket、TCP）
- Socket 操作
- 下载/上传任务

**限制**：
- 无 UI 操作
- 使用 `Dispatcher` 更新 UI

### Database 线程

**职责**：
- SQLite 操作
- 数据持久化
- 查询执行

**限制**：
- 无 UI 操作
- 使用 `Dispatcher` 更新 UI

### Timer 线程

**职责**：
- 定时器
- 周期性任务

**限制**：
- 无 UI 操作
- 仅允许短时间执行

---

## 线程安全

### 线程安全模块

这些模块设计为线程安全，可以从任何线程访问：

- **Core**：`Result`, `Error`, `Uuid`, `Version`, `Time`
- **Logging**：`Logger`, 所有 sinks
- **Configuration**：`Config`, `IConfiguration`
- **Network**：`HttpClient`, `WebSocket`, `TcpClient`
- **Storage**：`SqliteDatabase`, `Settings`, `ICache`
- **Async**：`ThreadPool`, `TaskRunner`, `Dispatcher`
- **Event**：`IEventBus`, `DefaultEventBus`
- **Utils**：所有工具函数

### 仅 GUI 线程模块

这些模块只能从 GUI 线程访问：

- **UI**：所有 widgets, `Theme`, `Style`, `Animation`
- **Base**：`BaseWidget`, `BaseDialog`, `BaseViewModel`

---

## 跨线程通信

### Dispatcher 模式

使用 `Dispatcher` 安全地在 GUI 线程上执行代码：

```cpp
// 从 worker 线程
sc::Dispatcher::invoke([]() {
    // 此代码在 GUI 线程上运行
    m_label->setText("Result ready");
});

// 带参数
sc::Dispatcher::invoke([result]() {
    m_label->setText(QString::number(result));
});

// 异步调用
sc::Dispatcher::invokeAsync([]() {
    // 非阻塞
});
```

### Signal-Slot 机制

Qt 的信号-槽机制自动处理跨线程通信：

```cpp
// Worker 类
class Worker : public QObject {
    Q_OBJECT
signals:
    void resultReady(int value);
};

// 在 GUI 线程
Worker* worker = new Worker();
connect(worker, &Worker::resultReady, this, [](int value) {
    m_label->setText(QString::number(value));
});
```

### Event Bus

事件总线支持同步和异步分发：

```cpp
// 同步分发（同一线程）
eventBus->publish("user.logged_in", userData);

// 异步分发（worker 线程 → GUI 线程）
eventBus->publishAsync("data.loaded", data);
```

---

## 线程池

### 全局线程池

- 由 `ThreadPool` 单例管理
- 根据 CPU 核心自动调整大小
- 默认：`QThread::idealThreadCount()` 个线程

### 任务提交

```cpp
auto future = sc::ThreadPool::instance()->submit([]() {
    return heavyComputation();
});
```

### 线程亲和性

- 任务分发到可用线程
- 除非显式设置，否则不保证线程亲和性
- 长时间运行的对象使用 `QObject::moveToThread()`

---

## 同步

### Mutex 指南

- 共享资源使用 `QMutex`
- 读多写少场景使用 `QReadWriteLock`
- 优先细粒度锁而非粗粒度锁

**示例**：
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

### 避免死锁

- 始终以相同顺序获取锁
- 避免嵌套锁
- 使用 `QMutexLocker` 自动解锁

---

## 禁止的做法

### 跨线程 Widget 访问

**禁止**：
```cpp
// 在 worker 线程
QWidget* widget = new QWidget(); // WRONG
widget->show(); // WRONG
```

**正确**：
```cpp
// 在 worker 线程
sc::Dispatcher::invoke([]() {
    QWidget* widget = new QWidget();
    widget->show();
});
```

### 阻塞 GUI 线程

**禁止**：
```cpp
// 在 GUI 线程
QThread::sleep(5000); // Blocks UI
```

**正确**：
```cpp
// 将繁重工作移到 worker 线程
auto future = sc::async([]() {
    return heavyOperation();
}).then([](Result<Data> result) {
    // 更新 UI
});
```

### 共享可变状态

**禁止**：
```cpp
// 无同步共享
int g_counter = 0;

// Thread 1
g_counter++;

// Thread 2
g_counter++;
```

**正确**：
```cpp
// 使用 atomic 或 mutex
std::atomic<int> g_counter{0};

// Thread 1
g_counter.fetch_add(1);

// Thread 2
g_counter.fetch_add(1);
```

---

## 线程模型审查清单

- ☑ UI 操作仅在 GUI 线程执行
- ☑ 使用 `Dispatcher` 进行跨线程 UI 更新
- ☑ 线程安全模块正确同步
- ☑ GUI 线程中无阻塞操作
- ☑ 使用 `QMutexLocker` 自动解锁
- ☑ 信号-槽连接处理跨线程
- ☑ 事件总线对 GUI 更新使用异步分发
- ☑ 无未同步的全局可变状态
- ☑ 任务提交到线程池而非手动线程
- ☑ 长时间运行的对象使用 `QObject::moveToThread()`