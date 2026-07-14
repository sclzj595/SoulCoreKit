# SoulCoreKit API Design Guide

## Overview

This document defines the API design principles, compatibility guarantees, and interface specifications for SoulCoreKit.

---

## API Stability

### Public API

**Definition**: All classes, functions, and constants exposed in the `include/soul/` directory.

**Guarantee**: Public APIs remain compatible across **minor versions**.

**Example**:
```
1.1 → 1.2: No breaking changes to public API
1.2 → 2.0: Breaking changes allowed
```

**Allowed Changes in Minor Versions**:
- Add new public classes
- Add new methods to existing classes
- Add new enum values
- Add new parameters with default values
- Deprecate (but not remove) existing API

**Prohibited Changes in Minor Versions**:
- Remove or rename public classes
- Remove or rename public methods
- Change method signatures
- Remove enum values
- Change default parameter values

### Private API

**Definition**: Internal implementation details not exposed in `include/soul/`.

**Guarantee**: None. Private APIs may change at any time.

---

## ABI Stability

### Public Export

**Definition**: Classes and functions marked for export via `SC_EXPORT`.

**Guarantee**: Once exported, ABI remains stable across **minor versions**.

**Prohibited Changes**:
- Reorder class members
- Change virtual table layout
- Remove or reorder virtual functions
- Change export symbols
- Change size of exported classes

### PImpl Pattern

**Recommended**: Use PImpl (Pointer to Implementation) for large classes to maintain ABI stability.

**Example**:
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
    // Can add more members without breaking ABI
};
```

---

## Interface Design

### Interface-First Principle

1. Define the interface before implementing
2. Use pure virtual functions
3. Include a virtual destructor

**Example**:
```cpp
class SC_EXPORT INetworkClient {
public:
    virtual ~INetworkClient() = default;
    virtual Result<HttpResponse> get(const QString& url) = 0;
    virtual Result<HttpResponse> post(const QString& url, const QByteArray& body) = 0;
};
```

### Interface Segregation

- Keep interfaces small and focused
- Avoid "fat" interfaces with unrelated methods

**Example**:
```cpp
// Good: Small, focused interfaces
class IReadable {
public:
    virtual Result<QByteArray> read() = 0;
};

class IWritable {
public:
    virtual Result<void> write(const QByteArray& data) = 0;
};

// Bad: Fat interface
class IStorage {
public:
    virtual Result<QByteArray> read() = 0;
    virtual Result<void> write(const QByteArray& data) = 0;
    virtual Result<void> delete() = 0;  // Not all storage needs delete
    virtual Result<void> backup() = 0;   // Not all storage needs backup
};
```

### Default Implementation

- Provide default implementations in separate classes
- Use composition for optional features

**Example**:
```cpp
class DefaultNetworkClient : public INetworkClient {
public:
    Result<HttpResponse> get(const QString& url) override;
    Result<HttpResponse> post(const QString& url, const QByteArray& body) override;
};
```

---

## Error Handling

### Result Pattern

- All non-trivial functions must return `Result<T>`
- Use `Error` for error information
- Never use `bool`, `int`, `-1`, or `nullptr` to indicate errors

**Example**:
```cpp
// Recommended
Result<User> getUser(const QString& id);

// Prohibited
User* getUser(const QString& id);
bool getUser(const QString& id, User& out);
int getUser(User* out); // Returns -1 on error
```

### Error Types

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

## Naming Conventions

### Class Names

- **Public**: PascalCase
- **Interfaces**: `I` prefix (e.g., `INetworkClient`)
- **Abstract**: `Abstract` prefix (e.g., `AbstractService`)

### Method Names

- **Getters**: `xxx()` or `getXxx()`
- **Setters**: `setXxx()`
- **Boolean**: `isXxx()`, `hasXxx()`, `canXxx()`
- **Actions**: `doXxx()`, `performXxx()`, `executeXxx()`
- **Factory**: `createXxx()`, `makeXxx()`

### Parameters

- **Input**: `const T&` for non-trivial types
- **Output**: Return via `Result<T>`, not out parameters
- **Optional**: `std::optional<T>`

---

## API Documentation

### Doxygen Requirements

- **Classes**: Purpose, key responsibilities
- **Methods**: Parameters, return values, exceptions
- **Enums**: Value descriptions
- **Constants**: Meaning and usage

### Example

```cpp
/**
 * @brief HTTP Client for making network requests
 * 
 * Provides async/sync HTTP request capabilities with timeout,
 * proxy support, and SSL/TLS.
 */
class SC_EXPORT HttpClient {
public:
    /**
     * @brief Send GET request
     * 
     * @param url Target URL
     * @param timeout Request timeout in milliseconds
     * @return Result<HttpResponse> Response or error
     * @throws None - errors are returned via Result
     */
    Result<HttpResponse> get(const QString& url, int timeout = 30000);
};
```

---

## Deprecation Policy

### Marking Deprecated

- Use `[[deprecated]]` attribute
- Add `@deprecated` Doxygen tag
- Provide migration guide in documentation

**Example**:
```cpp
/**
 * @brief Old method (deprecated)
 * 
 * @deprecated Use newMethod() instead
 */
[[deprecated("Use newMethod() instead")]]
void oldMethod();
```

### Removal Policy

- Deprecated API must remain for at least one major version
- Removal must be announced in release notes
- Migration guide must be provided

---

## Versioning

### Semantic Versioning

- **Major (X.y.z)**: Breaking API changes
- **Minor (x.Y.z)**: New features, backward-compatible
- **Patch (x.y.Z)**: Bug fixes only

### Version Query

Provide API to query current version:
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

## API Review Checklist

- ☑ Interface defined before implementation
- ☑ Uses `Result<T>` for error handling
- ☑ Follows naming conventions
- ☑ Has Doxygen documentation
- ☑ Compatible with previous minor versions
- ☑ ABI stable for exported classes
- ☑ Uses PImpl for large classes
- ☑ No raw pointers in public API
- ☑ Parameters use `const` references where appropriate
- ☑ Deprecated API marked with `[[deprecated]]`