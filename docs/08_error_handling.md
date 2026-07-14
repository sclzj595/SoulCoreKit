# SoulCoreKit Error Handling Specification

## Overview

This document defines the error handling strategy for SoulCoreKit, including error types, Result pattern, and error propagation rules.

---

## Result Pattern

### Core Concept

All non-trivial functions must return `Result<T>` instead of:
- `bool` (true/false)
- `int` (0/-1)
- `nullptr`
- Out parameters

### Result<T> Definition

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

### Usage Example

```cpp
Result<User> getUser(const QString& id) {
    auto data = m_storage->get(id);
    if (data.isErr()) {
        return Err(data.error());
    }
    return Ok(User::fromJson(data.value()));
}

// Caller
auto result = getUser("123");
if (result.isOk()) {
    SC_INFO() << "User: " << result.value().name();
} else {
    SC_ERROR() << "Failed to get user: " << result.error().message();
}
```

---

## Error Types

### ErrorCode Enum

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

### Error Class

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

## Error Propagation

### Chain of Errors

Use nested errors to preserve context:

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

### Error Handling at Boundaries

At UI boundary, convert errors to user-friendly messages:

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

## Prohibited Practices

### Boolean Return

**Prohibited**:
```cpp
bool getUser(const QString& id, User& out); // Returns false on error
```

**Correct**:
```cpp
Result<User> getUser(const QString& id);
```

### Integer Return

**Prohibited**:
```cpp
int createUser(const User& user); // Returns -1 on error
```

**Correct**:
```cpp
Result<void> createUser(const User& user);
```

### Null Pointer

**Prohibited**:
```cpp
User* getUser(const QString& id); // Returns nullptr on error
```

**Correct**:
```cpp
Result<User> getUser(const QString& id);
```

### Exception Throwing

**Prohibited**:
```cpp
User getUser(const QString& id) {
    if (!exists(id)) {
        throw std::runtime_error("User not found");
    }
}
```

**Correct**:
```cpp
Result<User> getUser(const QString& id) {
    if (!exists(id)) {
        return Err(Error(ErrorCode::NotFound, "User not found"));
    }
}
```

---

## Special Cases

### Void Return

For functions with no return value:

```cpp
Result<void> saveUser(const User& user);

// Usage
auto result = saveUser(user);
if (result.isErr()) {
    // Handle error
}
```

### Optional Values

Use `Result<std::optional<T>>` for optional results:

```cpp
Result<std::optional<User>> findUser(const QString& name);

// Usage
auto result = findUser("John");
if (result.isOk()) {
    auto user = result.value();
    if (user.has_value()) {
        // Found
    } else {
        // Not found (but no error)
    }
} else {
    // Error occurred
}
```

---

## Logging Errors

### Error Logging

Always log errors with context:

```cpp
auto result = operation();
if (result.isErr()) {
    SC_ERROR() << "Operation failed: " << result.error().toString();
}
```

### Error Context

Include relevant information in error messages:

```cpp
Result<void> connect(const QString& host, int port) {
    if (!validateHost(host)) {
        return Err(Error(ErrorCode::InvalidArgument, 
            QString("Invalid host: %1").arg(host)));
    }
}
```

---

## Error Handling Review Checklist

- ☑ Uses `Result<T>` for all non-trivial functions
- ☑ No `bool`, `int`, `-1`, or `nullptr` for error indication
- ☑ No exception throwing in public API
- ☑ Error messages contain useful context
- ☑ Nested errors preserve full context
- ☑ Errors are properly logged
- ☑ UI boundary converts errors to user-friendly messages
- ☑ Uses appropriate `ErrorCode` values
- ☑ `Result<void>` for functions with no return value
- ☑ `Result<std::optional<T>>` for optional results