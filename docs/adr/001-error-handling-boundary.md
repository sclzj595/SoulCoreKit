# ADR-001: Error Handling Boundary Rules

| | |
|---|---|
| **Status** | Accepted |
| **Date** | 2026-07-25 |
| **Module** | Core, All modules |

## Context

SoulCoreKit provides a rich `Result<T>` type (based on `std::variant<T, Error>`) that models successful values or typed errors. However, without clear boundary rules, API designers may inconsistently choose between `bool`, `Result<T>`, exceptions, or raw error codes, leading to:

- Ambiguity in API contracts (does `bool` false mean "not found" or "error"?)
- Inconsistent error propagation across module boundaries
- Lost diagnostic information when `bool` is used for operations that can fail for multiple reasons
- Overuse of exceptions for control flow in performance-critical paths

## Decision

All public APIs in SoulCoreKit must adhere to a strict three-tier classification:

### Tier 1 — Predicates (return `bool`)

Functions that answer a yes/no question about current state. No side effects, no allocation, no I/O. The `bool` return is semantically sufficient because the question has exactly two answers.

**Rules:**
- Must be `const` or have no meaningful side effects
- Must not fail for infrastructure reasons (I/O errors, network failures, etc.)
- If the state cannot be determined (e.g., database unreachable), the function must return a best-effort default or use `Result<bool>` instead

**Examples:**
```cpp
// --- Predicates: bool is correct ---
bool isTokenExpired() const;              // TokenManager
bool isAuthenticated() const;             // AuthManager
bool isConnected() const;                 // IDatabaseDriver
bool isEmpty() const;                     // Token
bool hasPermission(const QString&) const; // AuthManager
bool contains(const K& key) const;        // ICache
bool validateFormat(const QString&);      // TokenManager (format check only, no I/O)
bool isFinished() const;                  // Future
bool isCancelled() const;                 // Future / CancelableTask
```

### Tier 2 — Operations (return `Result<T>`)

Functions that perform work with a typed return value. The operation may fail for multiple distinct reasons (not found, permission denied, network error, validation failure, etc.) each requiring different handling by the caller.

**Rules:**
- All operations that allocate, perform I/O, or interact with external systems must return `Result<T>` or `Result<void>`
- The `Error` object must carry a descriptive `ErrorCode` and human-readable `message`
- Callers must check `isOk()` before calling `unwrap()` — never unwrap without checking
- Use `map()`, `andThen()`, and `orElse()` to chain operations instead of nested if-checks where appropriate

**Examples:**
```cpp
// --- Operations: Result<T> is mandatory ---
Result<UserInfo> login(const QString& username, const QString& password);
Result<QString> refreshToken();
Result<T> findById(const QString& id);
Result<std::vector<T>> findAll();
Result<T> save(const T& entity);
Result<void> removeById(const QString& id);
Result<void> open(const ConnectionConfig& config);
Result<QueryResult> executeQuery(const QString& sql, const std::vector<QVariant>& params);
Result<void> put(const K& key, const V& value);
Result<V> get(const K& key);
Result<void> publish(const std::string& topic, const T& data);
Result<void> init();
```

### Tier 3 — Validation with Diagnostics (return `Result<T, ValidationError>`)

Functions that validate user-supplied input and need to report *all* violations, not just the first one. This tier extends the `Result` pattern for multi-error reporting.

**Rules:**
- Use when validation must report multiple errors simultaneously (e.g., form validation)
- `ValidationError` extends `Error` with a list of `ValidationIssue` items
- For single-error validation (e.g., "is this token format valid?"), use Tier 1 (`bool`) or Tier 2 (`Result<T>`)

**Examples:**
```cpp
// --- Validation with diagnostics ---
struct ValidationIssue {
    QString field;
    ErrorCode code;
    QString message;
};

struct ValidationError : Error {
    std::vector<ValidationIssue> issues;
};

Result<UserRegistration, ValidationError> validateRegistrationForm(const FormData& form);
Result<Config, ValidationError> validateConfigSchema(const QJsonObject& config);
```

## Consequences

### Positive
- **Clear API contracts**: Reviewers can immediately understand failure semantics from the return type
- **Consistent error handling**: No more guessing whether `false` means "not found" or "error"
- **Better diagnostics**: `Result<T>` preserves error codes and messages through the call stack
- **No exceptions**: The framework avoids C++ exceptions as a control-flow mechanism in hot paths
- **Chainable operations**: `andThen()` and `orElse()` enable clean error-propagation pipelines

### Negative
- **Slightly more verbose**: Callers must check `isOk()` / `unwrap()` rather than `if (!ptr)`
- **Template complexity**: `Result<T>` is a templated type, which may increase compile times
- **Three-tier classification overhead**: Some functions may be borderline (e.g., `init()` has side effects but returns `bool` in legacy code)

## Enforcement

1. **Code review checklist**: Every public API must be classified as Tier 1, 2, or 3 during review
2. **Header hygiene**: New `bool`-returning functions in public headers must be justified as Tier 1 predicates
3. **Static analysis** (future): A Clang-Tidy check can flag `bool` returns in functions that call I/O or allocation APIs
4. **Documentation**: The `docs/08_error_handling.md` document provides a comprehensive guide with examples

## Validation Flow Diagram

```mermaid
flowchart TD
    A[New API Design] --> B{Is it a predicate?}
    B -->|Yes| C[Return bool]
    B -->|No| D{Has multiple<br/>failure modes?}
    D -->|No| E{Is validation with<br/>multiple issues?}
    D -->|Yes| F[Return Result T]
    E -->|Yes| G[Return Result T, ValidationError]
    E -->|No| F

    style C fill:#c8e6c9,color:#1a5e20
    style F fill:#bbdefb,color:#0d47a1
    style G fill:#fff3e0,color:#e65100
    style B fill:#f3e5f5,color:#7b1fa2
    style D fill:#f3e5f5,color:#7b1fa2
    style E fill:#f3e5f5,color:#7b1fa2