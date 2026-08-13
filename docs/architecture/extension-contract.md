# SoulCoreKit Extension Contract v2.9.3

> **状态**: Frozen  
> **适用范围**: 所有 Extensions 层模块  
> **最后更新**: 2026-08-08

---

## 1. 概述

Extension Contract 定义 SoulCoreKit Extensions 层所有模块必须遵守的契约。
这些契约确保每个 Extension 的生命周期、线程安全、错误处理和依赖边界保持一致，
避免每个模块各自发明一套规则。

---

## 2. 模块验收矩阵

| 契约项 | Configuration | MessageBus | Cache | Discovery |
|--------|:---:|:---:|:---:|:---:|
| **API 最小化** | ✅ | ✅ | ✅ | ✅ |
| **Result\<T\> 统一** | ✅ | ✅ | ✅ | ✅ |
| **生命周期四阶段** | ✅ | ✅ | ✅ | ✅ |
| **shutdown 幂等** | ✅ | ✅ | ✅ | ✅ |
| **线程安全** | ✅ | ✅ | ✅ | ✅ |
| **callback 非重入** | N/A | ✅ | N/A | ✅ |
| **Health 反向注册** | ✅ | N/A | ✅ | ✅ |
| **Config 注入非依赖** | N/A | ✅ | ✅ | ✅ |
| **外部依赖 Adapter 隔离** | ✅ | ✅ | ✅ | ✅ |
| **四层依赖不违规** | ✅ | ✅ | ✅ | ✅ |

---

## 3. 契约详情

### 3.1 API 最小化

- 公共接口 ≤ 10 个方法
- 不暴露实现细节
- 不使用 God Object 模式

### 3.2 Result\<T\> 统一

- 所有可失败操作返回 `Result<T>`，复用 `sc::Error`
- 不创建模块专属 Error 类型
- 失败语义明确区分 (Miss vs Error, Unavailable vs Timeout)

### 3.3 生命周期四阶段

```
initialize() → start() → stop() → shutdown()
```

- `initialize/start` 返回 `Result<void>` (可失败)
- `stop/shutdown` 返回 `void noexcept` (保证执行)
- `shutdown` 幂等

### 3.4 线程安全

- 明确标注: Thread-safe / Thread-confined / Immutable
- 锁内禁止执行用户回调
- 锁内禁止 emit signal

### 3.5 Callback 非重入

- watch/subscribe callback 禁止在回调中 register/unregister
- 文档明确标注

### 3.6 Health 反向注册

```
Adapter → IHealthIndicator → HealthAggregator
```

- Extension Core 不依赖 Health
- Adapter 可选提供 HealthIndicator

### 3.7 Config 注入非依赖

```
Application → Options → Extension
```

- Extension 不依赖具体 Config 实现
- 配置通过构造/Options 注入

### 3.8 外部依赖 Adapter 隔离

- Core/Infrastructure 不引入第三方 SDK
- Adapter 编译可选 (CMake option OFF 默认)

### 3.9 四层依赖不违规

```
Core → Infrastructure → Extensions → Application
```

- 禁止 Core → Extensions
- 禁止 Infrastructure → Extensions
