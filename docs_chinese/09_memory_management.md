# SoulCoreKit 内存管理规范

> **翻译状态**: 待翻译 — 请参考 [英文原版](../docs/09_memory_management.md)

## 概述

本文档定义 SoulCoreKit 的内存管理策略，包括所有权规则、智能指针使用和生命周期管理。

---

## 所有权规则

### QObject 层级

**规则**: QObject 派生类使用 Qt 的父子层级进行所有权管理。

```cpp
// 父对象管理子对象
QWidget* parent = new QWidget();
QPushButton* button = new QPushButton(parent); // parent 拥有 button

// 当 parent 销毁时，button 自动销毁
delete parent;
```

### 非 QObject 对象

**规则**: 使用 `std::unique_ptr` 进行独占所有权管理。

```cpp
class Service {
private:
    std::unique_ptr<Repository> m_repository;
};
```

### 共享所有权

**规则**: 使用 `std::shared_ptr` 仅在需要共享所有权时，优先使用 `unique_ptr`。

```cpp
// 仅在多个所有者需要时使用 shared_ptr
std::shared_ptr<Config> config = std::make_shared<Config>();
```

---

## RAII 原则

所有资源管理必须遵循 RAII（Resource Acquisition Is Initialization）：

- 文件句柄 → `std::fstream` / `QFile`
- 网络连接 → `QTcpSocket`（QObject 层级管理）
- 数据库连接 → 连接池管理
- 互斥锁 → `std::mutex` / `QMutex`
- 内存 → 智能指针

---

## 禁止事项

- ❌ 禁止裸指针管理资源所有权
- ❌ 禁止手动 `new`/`delete` 配对
- ❌ 禁止 `QObject` 使用 `std::unique_ptr`（与 Qt 父子层级冲突）
- ❌ 禁止在析构函数中抛出异常

---

> **完整内容请参考**: [英文原版 Memory Management Specification](../docs/09_memory_management.md)