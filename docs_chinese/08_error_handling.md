# SoulCoreKit 错误处理规范

## 概述

本文档定义了 SoulCoreKit 的错误处理策略，包括错误类型、Result 模式和错误传播规则。

---

## Result 模式

### 核心概念

所有非平凡函数必须返回 `Result<T>`，而不是：
- `bool` (true/false)
- `int` (0/-1)
- `nullptr`
- 输出参数

### Result<T> 定义

```cpp
template <typename T>
class Result {
public:
    static Result Ok(T value);
    static Result Err(Error error);
    
    bool isOk() const;
    bool isErr() const;
    
    T& value();
    const T& value() const;
    
    Error& error();
    const Error& error() const;
    
    template <typename U>
    Result<U> map(std::function<U(T)> func);
    
    template <typename U>
    Result<U> flatMap(std::function<Result<U>(T)> func);
};
```

### 使用示例

```cpp
Result<User> getUser(const QString& id) {
    auto data = m_storage->get(id);
    if (data.isErr()) {
        return Err(data.error());
    }
    return Ok(User::fromJson(data.value()));
}

// 调用方
auto result = getUser("123");
if (result.isOk()) {
    SC_INFO() << "User: " << result.value().name();
} else {
    SC_ERROR() << "Failed to get user: " << result.error().message();
}
```

---

## 错误类型

### ErrorCode 枚举

```cpp
enum class ErrorCode {
    Ok = 0,
    
    NotFound = 100,
    AlreadyExists = 101,
    InvalidArgument = 102,
    InvalidState = 103,
    PermissionDenied = 104,
    Unauthorized = 105,
    
    NetworkError = 200,
    Timeout = 201,
    ConnectionRefused = 202,
    SSLHandshakeFailed = 203,
    
    DatabaseError = 300,
    QueryFailed = 301,
    ConstraintViolation = 302,
    
    FileError = 400,
    FileNotFound = 401,
    FileReadError = 402,
    FileWriteError = 403,
    
    InternalError = 500,
    NotImplemented = 501,
    OutOfMemory = 502,
};
```

### Error 类

```cpp
class Error {
public:
    Error(ErrorCode code, const QString& message);
    Error(ErrorCode code, const QString& message, const Error& cause);
    
    ErrorCode code() const;
    QString message() const;
    std::optional<Error> cause() const;
    
    QString toString() const;
};
```

---

## 错误传播

### 错误链

使用嵌套错误保留上下文：

```cpp
Result<User> fetchUser(const QString& id) {
    auto response = m_httpClient->get("/users/" + id);
    if (response.isErr()) {
        return Err(Error(ErrorCode::NetworkError, 
            "Failed to fetch user", 
            response.error()));
    }
    
    auto data = parseJson(response.value().body());
    if (data.isErr()) {
        return Err(Error(ErrorCode::InternalError, 
            "Failed to parse user data", 
            data.error()));
    }
    
    return Ok(User::fromJson(data.value()));
}
```

### 边界错误处理

在 UI 边界，将错误转换为用户友好的消息：

```cpp
void onFetchUserClicked() {
    auto result = m_userService->fetchUser(m_idInput->text());
    if (result.isErr()) {
        auto error = result.error();
        switch (error.code()) {
            case ErrorCode::NetworkError:
                Toast::show("网络连接失败，请检查网络");
                break;
            case ErrorCode::NotFound:
                Toast::show("用户不存在");
                break;
            default:
                Toast::show("操作失败: " + error.message());
                break;
        }
        SC_ERROR() << "User fetch failed: " << error.toString();
    }
}
```

---

## 禁止的做法

### 布尔返回

**禁止**：
```cpp
bool getUser(const QString& id, User& out); // 错误时返回 false
```

**正确**：
```cpp
Result<User> getUser(const QString& id);
```

### 整数返回

**禁止**：
```cpp
int createUser(const User& user); // 错误时返回 -1
```

**正确**：
```cpp
Result<void> createUser(const User& user);
```

### 空指针

**禁止**：
```cpp
User* getUser(const QString& id); // 错误时返回 nullptr
```

**正确**：
```cpp
Result<User> getUser(const QString& id);
```

### 抛出异常

**禁止**：
```cpp
User getUser(const QString& id) {
    if (!exists(id)) {
        throw std::runtime_error("User not found");
    }
}
```

**正确**：
```cpp
Result<User> getUser(const QString& id) {
    if (!exists(id)) {
        return Err(Error(ErrorCode::NotFound, "User not found"));
    }
}
```

---

## 特殊情况

### 无返回值

对于无返回值的函数：

```cpp
Result<void> saveUser(const User& user);

// 使用
auto result = saveUser(user);
if (result.isErr()) {
    // 处理错误
}
```

### 可选值

使用 `Result<std::optional<T>>` 表示可选结果：

```cpp
Result<std::optional<User>> findUser(const QString& name);

// 使用
auto result = findUser("John");
if (result.isOk()) {
    auto user = result.value();
    if (user.has_value()) {
        // 找到
    } else {
        // 未找到（但没有错误）
    }
} else {
    // 发生错误
}
```

---

## 错误日志

### 错误日志

始终记录带有上下文的错误：

```cpp
auto result = operation();
if (result.isErr()) {
    SC_ERROR() << "Operation failed: " << result.error().toString();
}
```

### 错误上下文

在错误消息中包含相关信息：

```cpp
Result<void> connect(const QString& host, int port) {
    if (!validateHost(host)) {
        return Err(Error(ErrorCode::InvalidArgument, 
            QString("Invalid host: %1").arg(host)));
    }
}
```

---

## 错误处理审查清单

- ☑ 所有非平凡函数使用 `Result<T>`
- ☑ 不使用 `bool`, `int`, `-1`, 或 `nullptr` 表示错误
- ☑ 公共 API 中不抛出异常
- ☑ 错误消息包含有用上下文
- ☑ 嵌套错误保留完整上下文
- ☑ 错误已正确记录
- ☑ UI 边界将错误转换为用户友好消息
- ☑ 使用适当的 `ErrorCode` 值
- ☑ 无返回值函数使用 `Result<void>`
- ☑ 可选结果使用 `Result<std::optional<T>>`