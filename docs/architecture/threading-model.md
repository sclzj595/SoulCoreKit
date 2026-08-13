# SoulCoreKit Threading Model v2.6.0

> **状态**: Frozen  
> **适用范围**: CS + BS 双场景  
> **最后更新**: 2026-08-08

---

## 1. 概述

SoulCoreKit 同时服务于 CS (Qt Desktop/Server) 和 BS (Web Backend) 场景，线程模型的正确性直接影响框架的稳定性。本文档定义所有核心组件的线程归属、跨线程通信规则和并发安全保证。

---

## 2. 线程类型定义

| 线程类型 | 典型场景 | 职责 | 限制 |
|----------|----------|------|------|
| **GUI Thread** | CS Client | 所有 UI 操作、事件处理 | 禁止阻塞操作 (>50ms) |
| **Worker Thread** | CS/BS | CPU 密集型任务、后台处理 | 禁止 UI 操作 |
| **Network Thread** | CS/BS | HTTP/WS/TCP I/O | 禁止 UI 操作 |
| **Database Thread** | CS/BS | SQL 执行、数据持久化 | 禁止 UI 操作 |
| **Timer Thread** | CS/BS | 定时器、周期性任务 | 短执行时间 (<10ms) |
| **Main Thread** | BS Backend | QCoreApplication 事件循环 | BS 场景无 GUI，主线程 = 事件循环线程 |

---

## 3. 强制规则 (Must Follow)

### Rule 1: QObject 线程亲和性

```
QObject 所属线程 = 创建它的线程 (thread affinity)
QObject 的事件处理在所属线程的事件循环中执行
```

- `moveToThread()` 可以迁移 QObject，但必须在迁移**前**断开所有跨线程信号连接
- 禁止在非所属线程中直接调用 QObject 的非线程安全方法

### Rule 2: 禁止跨线程直接调用

```cpp
// ❌ 错误: Worker 线程直接操作 UI
void WorkerThread::process() {
    m_label->setText("Done");  // UNDEFINED BEHAVIOR
}

// ✅ 正确: 通过 Dispatcher 投递到 GUI 线程
void WorkerThread::process() {
    Dispatcher::invoke([this] {
        m_label->setText("Done");
    });
}
```

### Rule 3: 锁内禁止执行用户回调

```cpp
// ❌ 错误: 持锁回调 — 死锁风险
void ConnectionPool::forEachConnection(std::function<void(Connection*)> fn) {
    std::lock_guard lock(m_mutex);
    for (auto& conn : m_connections) {
        fn(conn.get());  // 用户回调可能再次进入本类获取同一把锁
    }
}

// ✅ 正确: 快照后释放锁再回调
void ConnectionPool::forEachConnection(std::function<void(Connection*)> fn) {
    std::vector<Connection*> snapshot;
    {
        std::lock_guard lock(m_mutex);
        for (auto& conn : m_connections) {
            snapshot.push_back(conn.get());
        }
    }
    for (auto* conn : snapshot) {
        fn(conn);
    }
}
```

### Rule 4: 锁内禁止 emit 可能触发外部逻辑的 signal

```cpp
// ❌ 错误: 持锁 emit — 外部槽函数可能尝试获取同一把锁
void EventBus::dispatch(Event* event) {
    std::lock_guard lock(m_mutex);
    emit eventDispatched(event);  // 死锁风险
}

// ✅ 正确: 先释放锁再 emit
void EventBus::dispatch(Event* event) {
    Event* copy = event->clone();
    {
        std::lock_guard lock(m_mutex);
        m_queue.push_back(copy);
    }
    emit eventDispatched(copy);
}
```

### Rule 5: Worker 不直接操作 UI

所有 UI 更新必须通过以下方式之一投递到 GUI 线程:
- `Dispatcher::invoke()` — 同步/异步投递
- `Qt::QueuedConnection` — 跨线程信号槽
- `QMetaObject::invokeMethod(obj, method, Qt::QueuedConnection, ...)`

### Rule 6: DB Connection 默认线程隔离

- 每个线程持有独立的数据库连接
- 禁止跨线程共享 `QSqlDatabase` 连接
- `ConnectionPool` 按线程 ID 分配连接

### Rule 7: Task Cancellation 必须明确 happens-before

```cpp
// ✅ 正确: atomic flag + 明确的内存序
class CancelableTask {
    std::atomic<bool> m_cancelled{false};

    void cancel() {
        m_cancelled.store(true, std::memory_order_release);
    }

    void execute() {
        while (!m_cancelled.load(std::memory_order_acquire)) {
            doWork();
        }
    }
};
```

---

## 4. 组件线程安全分类

### 4.1 线程安全 (可在任意线程并发使用)

| 组件 | 线程安全保证 |
|------|-------------|
| `Result<T>` | 值语义，天然线程安全 |
| `Error` | 不可变值对象 |
| `Uuid` | `thread_local` mt19937 |
| `Version` | 编译时常量 |
| `Time` | 无状态工具函数 |
| `Logger` (spdlog) | spdlog 内置线程安全 |
| `EventBus` | 内部 mutex 保护 |
| `DI Container` | DCLP + shared_ptr 保护 |
| `ThreadPool` | 内部同步原语 |
| `Configuration` (读取) | 读操作线程安全 |

### 4.2 线程亲和 (必须在创建线程使用)

| 组件 | 线程约束 |
|------|----------|
| `QWidget` 及其子类 | GUI Thread 独占 |
| `QSqlDatabase` 连接 | 创建线程独占 |
| `QNetworkAccessManager` | 创建线程的事件循环 |
| `QTimer` | 创建线程的事件循环 |

### 4.3 需要外部同步

| 组件 | 说明 |
|------|------|
| `Configuration` (写入) | 写操作需外部同步 |
| `Cache` (Memory) | 并发读写需外部锁或使用 `MultiLevelCache` |
| `Storage` (File) | 文件级锁由 OS 提供 |
| `Settings` | QSettings 内部有锁但写操作应序列化 |

---

## 5. 跨线程通信方式

| 方式 | 适用场景 | 同步/异步 |
|------|----------|-----------|
| `Dispatcher::invoke()` | 投递到 GUI 线程 | 同步 |
| `Dispatcher::invokeAsync()` | 投递到 GUI 线程 | 异步 |
| `Qt::QueuedConnection` | QObject 跨线程信号槽 | 异步 |
| `Qt::BlockingQueuedConnection` | QObject 跨线程信号槽 | 同步 |
| `EventBus::post()` | 异步事件分发 | 异步 |
| `EventBus::send()` | 同步事件分发 | 同步 |
| `Future<T>::then()` | 异步任务链 | 异步 |
| `Promise<T>` / `QPromise<T>` | 异步结果传递 | 异步 |

---

## 6. 生命周期阶段线程约束

| 阶段 | 执行线程 | 约束 |
|------|----------|------|
| `initialize()` | 调用方线程 (通常主线程) | 同步执行，禁止启动工作线程 |
| `start()` | 调用方线程 (通常主线程) | 可启动工作线程，禁止阻塞等待 |
| `stop()` | 调用方线程 (通常主线程) | 发送停止信号，等待工作线程 join |
| `shutdown()` | 调用方线程 (通常主线程) | 所有工作线程必须已停止 |

---

## 7. BS 场景特殊考虑

BS 场景无 GUI Thread，主线程即 QCoreApplication 事件循环线程:

```
Main Thread (event loop)
    ├── HTTP Worker Pool
    │   └── 处理 HTTP 请求
    ├── Database Pool
    │   └── 执行 SQL 查询
    └── Async Task Runner
        └── 后台任务
```

- BS 场景下 `Dispatcher::invoke()` 投递到主线程事件循环
- 不需要 Qt::Widgets 模块
- 所有 I/O 操作应在独立线程池中执行

---

## 8. 违反规则的检测方法

| 方法 | 检测内容 |
|------|----------|
| ThreadSanitizer (TSan) | 数据竞争 |
| `Q_ASSERT_X(QThread::currentThread() == thread(), ...)` | QObject 线程亲和性违规 |
| CI TSan workflow | 自动化竞争检测 |

构建 TSan 版本:
```bash
cmake -S . -B build-tsan -DENABLE_TSAN=ON -DBUILD_TESTS=ON
cmake --build build-tsan
cd build-tsan && ctest --output-on-failure
```
