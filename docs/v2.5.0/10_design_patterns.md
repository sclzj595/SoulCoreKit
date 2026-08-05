# 10 — 设计模式

> **版本**: v2.5.0
> **日期**: 2026-08-05

---

## 10.1 设计模式应用清单

| 模式 | 类型 | 应用场景 | 实现位置 |
|------|------|----------|----------|
| **Singleton** | 创建型 | 全局横切关注点 (Theme, ErrorHandler, Container, IconManager) | `Singleton<T>` CRTP, Meyer's Singleton |
| **Factory Method** | 创建型 | 网络工厂, MQ 工厂, Repository 工厂, 数据库驱动工厂 | `NetworkFactory`, `MQFactory`, `RepositoryFactory`, `DatabaseDriverFactory` |
| **Builder** | 创建型 | 查询构建器 | `QueryBuilder` 链式 API |
| **Template Method** | 行为型 | BaseRepository, Entity 生命周期 | `BaseRepository<T>`, `Entity<T>::beforeInsert/beforeUpdate` |
| **Strategy** | 行为型 | 网络协议适配器, 数据库方言, 限流算法 | `INetwork`, `ISqlDialect`, `RateLimiter::Algorithm` |
| **Observer** | 行为型 | 事件总线, Qt Signal/Slot | `EventBus`, `TypedEventBus<T>`, `CsViewModel` |
| **Decorator** | 结构型 | 缓存仓库 | `CachedRepository<T>` |
| **Chain of Responsibility** | 行为型 | HTTP 拦截器链, 中间件链 | `IInterceptor`, `IMiddleware` |
| **Proxy** | 结构型 | RPC 客户端代理 | `ClientProxy` |
| **Facade** | 结构型 | Scaffold 对 Application 的封装 | `Scaffold` |
| **Composite** | 结构型 | 复合日志 Sink, 组合表单校验器 | `CompositeSink`, `CompositeFormValidator` |
| **CRTP** | 惯用法 | Entity 静态多态, 数据库驱动复用 | `Entity<Derived>`, `DatabaseDriverBase<T>` |
| **Adapter** | 结构型 | 协议适配器 (WebSocket/TCP/MQTT/BLE/Serial/NamedPipe) | `*ClientAdapter` 系列 |
| **State** | 行为型 | 连接状态机, 应用状态机, 熔断器 | `ConnectionStateMachine`, `ApplicationState`, `CircuitBreaker` |

---

## 10.2 关键模式详解

### Singleton — 受控生命周期

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

**适用场景**: 跨模块的全局横切关注点，生命周期与 Application 绑定。
**不适合**: 业务 Service（应通过 DI 容器管理）、数据 Repository（应通过 ApplicationContext 获取）。

### CRTP — 编译期多态

```cpp
// Entity 静态多态
template<typename Derived>
class Entity {
    QVariant getProperty(const QString& name) const {
        return static_cast<const Derived*>(this)->getPropertyImpl(name);
    }
};

// 数据库驱动复用
template<typename Derived>
class DatabaseDriverBase {
    // executeQuery/executeUpdate/事务管理 公共逻辑
};
```

### Strategy — 算法族可替换

```cpp
// 数据库方言
class ISqlDialect {
    virtual QString placeholder(int index) = 0;
    virtual QString limitOffset(int limit, int offset) = 0;
};

// 限流算法
enum class Algorithm { TokenBucket, SlidingWindow };
```

### Decorator — 动态增强

```cpp
// 缓存装饰器
template<typename T>
class CachedRepository : public IRepository<T> {
    std::shared_ptr<IRepository<T>> m_delegate;
    std::shared_ptr<ICache> m_cache;
};
```

---

## 10.3 架构原则

| 原则 | 说明 |
|------|------|
| **SOLID** | 单一职责、开闭原则、里氏替换、接口隔离、依赖反转 |
| **DDD** | 聚合根、实体、值对象、限界上下文 |
| **CQRS** | 读写分离 (ReadWriteRepository) |
| **分层架构** | Foundation → Application → CS/Web |
| **单向依赖** | View → ViewModel → Controller → Service → Repository |