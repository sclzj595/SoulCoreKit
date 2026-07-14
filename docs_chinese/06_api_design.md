# SoulCoreKit API 设计指南

## 概述

本文档定义了 SoulCoreKit 的 API 设计原则、兼容性保证和接口规范。

---

## API 稳定性

### 公共 API

**定义**：`include/soul/` 目录中暴露的所有类、函数和常量。

**保证**：公共 API 在**次要版本**间保持兼容。

**示例**：
```
1.1 → 1.2: 公共 API 无破坏性变更
1.2 → 2.0: 允许破坏性变更
```

**次要版本允许的变更**：
- 添加新的公共类
- 向现有类添加新方法
- 添加新的枚举值
- 添加带默认值的新参数
- 弃用（但不删除）现有 API

**次要版本禁止的变更**：
- 删除或重命名公共类
- 删除或重命名公共方法
- 更改方法签名
- 删除枚举值
- 更改默认参数值

### 私有 API

**定义**：未在 `include/soul/` 中暴露的内部实现细节。

**保证**：无。私有 API 可能随时变更。

---

## ABI 稳定性

### 公共导出

**定义**：通过 `SC_EXPORT` 标记为导出的类和函数。

**保证**：一旦导出，ABI 在**次要版本**间保持稳定。

**禁止的变更**：
- 重排类成员
- 更改虚表布局
- 删除或重排虚函数
- 更改导出符号
- 更改导出类的大小

### PImpl 模式

**推荐**：大型类使用 PImpl（Pointer to Implementation）模式以保持 ABI 稳定性。

**示例**：
```cpp
// header
class SC_EXPORT Widget {
public:
    Widget();
    ~Widget();
    void setText(const QString& text);
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

// source
class Widget::Impl {
public:
    QString text;
    // 可以添加更多成员而不破坏 ABI
};
```

---

## 接口设计

### 接口优先原则

1. 在实现之前定义接口
2. 使用纯虚函数
3. 包含虚析构函数

**示例**：
```cpp
class SC_EXPORT INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual Result<HttpResponse> get(const QString& url) = 0;
    virtual Result<HttpResponse> post(const QString& url, const QByteArray& body) = 0;
};
```

### 接口隔离

- 保持接口小而专注
- 避免"胖"接口包含不相关的方法

**示例**：
```cpp
// 好：小而专注的接口
class IReadable {
public:
    virtual Result<QByteArray> read() = 0;
};

class IWritable {
public:
    virtual Result<void> write(const QByteArray& data) = 0;
};

// 坏：胖接口
class IStorage {
public:
    virtual Result<QByteArray> read() = 0;
    virtual Result<void> write(const QByteArray& data) = 0;
    virtual Result<void> delete() = 0;  // 并非所有存储都需要 delete
    virtual Result<void> backup() = 0;   // 并非所有存储都需要 backup
};
```

### 默认实现

- 在单独的类中提供默认实现
- 使用组合实现可选功能

**示例**：
```cpp
class DefaultNetworkClient : public INetworkClient {
public:
    Result<HttpResponse> get(const QString& url) override;
    Result<HttpResponse> post(const QString& url, const QByteArray& body) override;
};
```

---

## 错误处理

### Result 模式

- 所有非平凡函数必须返回 `Result<T>`
- 使用 `Error` 传递错误信息
- 绝不使用 `bool`, `int`, `-1`, 或 `nullptr` 表示错误

**示例**：
```cpp
// 推荐
Result<User> getUser(const QString& id);

// 禁止
User* getUser(const QString& id);
bool getUser(const QString& id, User& out);
int getUser(User* out); // 错误时返回 -1
```

### 错误类型

```cpp
enum class ErrorCode {
    Ok = 0,
    NotFound,
    NetworkError,
    PermissionDenied,
    InvalidArgument,
    InternalError
};
```

---

## 命名规范

### 类名

- **公共类**：PascalCase
- **接口**：`I` 前缀（如 `INetworkClient`）
- **抽象类**：`Abstract` 前缀（如 `AbstractService`）

### 方法名

- **Getters**：`xxx()` 或 `getXxx()`
- **Setters**：`setXxx()`
- **布尔值**：`isXxx()`, `hasXxx()`, `canXxx()`
- **动作**：`doXxx()`, `performXxx()`, `executeXxx()`
- **工厂**：`createXxx()`, `makeXxx()`

### 参数

- **输入**：非平凡类型使用 `const T&`
- **输出**：通过 `Result<T>` 返回，不使用输出参数
- **可选**：`std::optional<T>`

---

## API 文档

### Doxygen 要求

- **类**：目的、主要职责
- **方法**：参数、返回值、异常
- **枚举**：值描述
- **常量**：含义和用法

### 示例

```cpp
/**
 * @brief 用于发起网络请求的 HTTP 客户端
 * 
 * 提供异步/同步 HTTP 请求能力，支持超时、
 * 代理和 SSL/TLS。
 */
class SC_EXPORT HttpClient {
public:
    /**
     * @brief 发送 GET 请求
     * 
     * @param url 目标 URL
     * @param timeout 请求超时时间（毫秒）
     * @return Result<HttpResponse> 响应或错误
     * @throws None - 错误通过 Result 返回
     */
    Result<HttpResponse> get(const QString& url, int timeout = 30000);
};
```

---

## 弃用策略

### 标记弃用

- 使用 `[[deprecated]]` 属性
- 添加 `@deprecated` Doxygen 标签
- 在文档中提供迁移指南

**示例**：
```cpp
/**
 * @brief 旧方法（已弃用）
 * 
 * @deprecated 使用 newMethod() 替代
 */
[[deprecated("Use newMethod() instead")]]
void oldMethod();
```

### 删除策略

- 已弃用的 API 必须保留至少一个主版本
- 删除必须在发布说明中公告
- 必须提供迁移指南

---

## 版本控制

### 语义化版本

- **主版本 (X.y.z)**：破坏性 API 变更
- **次版本 (x.Y.z)**：新功能，向后兼容
- **补丁版本 (x.y.Z)**：仅修复 bug

### 版本查询

提供 API 查询当前版本：
```cpp
class Version {
public:
    static int major();
    static int minor();
    static int patch();
    static QString toString();
};
```

---

## API 审查清单

- ☑ 接口在实现之前定义
- ☑ 使用 `Result<T>` 进行错误处理
- ☑ 遵循命名规范
- ☑ 有 Doxygen 文档
- ☑ 与先前次要版本兼容
- ☑ 导出类的 ABI 稳定
- ☑ 大型类使用 PImpl
- ☑ 公共 API 中无裸指针
- ☑ 参数在适当位置使用 `const` 引用
- ☑ 已弃用 API 标记 `[[deprecated]]`