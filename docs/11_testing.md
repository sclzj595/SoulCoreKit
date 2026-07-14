# SoulCoreKit Testing Specification

## Overview

This document defines the testing strategy for SoulCoreKit, including test types, coverage requirements, and test infrastructure.

---

## Test Types

### Unit Tests

- **Scope**: Individual classes and functions
- **Isolation**: Use mocks for dependencies
- **Framework**: Qt Test
- **Location**: `tests/unit/`

### Integration Tests

- **Scope**: Multiple components working together
- **Examples**: Network + Storage workflows
- **Location**: `tests/integration/`

### UI Tests

- **Scope**: UI components and interactions
- **Framework**: Qt Test with QWidget testing
- **Location**: `tests/ui/`

### Regression Tests

- **Scope**: Bug fixes and backward compatibility
- **Trigger**: Run on every PR
- **Location**: `tests/regression/`

---

## Test Coverage Requirements

| Module | Target Coverage | Status |
|--------|----------------|--------|
| **Core** | >90% | Required |
| **Base** | >80% | Required |
| **Logging** | >90% | Required |
| **Configuration** | >80% | Required |
| **Network** | >70% | Required |
| **Storage** | >80% | Required |
| **Async** | >70% | Required |
| **Event** | >80% | Required |
| **UI** | >70% | Required |
| **Auth** | >70% | Required |
| **Utils** | >80% | Required |

---

## Test Structure

### Directory Layout

```
tests/
├── CMakeLists.txt
├── unit/
│   ├── core/
│   ├── ui/
│   ├── network/
│   └── ...
├── integration/
│   ├── network_storage/
│   └── ...
├── ui/
│   ├── button_test.cpp
│   ├── card_test.cpp
│   └── ...
└── regression/
    ├── issue_123_test.cpp
    └── ...
```

### Test File Naming

- **Rule**: `<component>_test.cpp`
- **Example**: `button_test.cpp`, `http_client_test.cpp`

---

## Test Framework

### Qt Test

```cpp
#include <QtTest/QtTest>
#include "soul/core/result.h"

class ResultTest : public QObject {
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();
    
    void testOk();
    void testErr();
    void testMap();
    void testFlatMap();
};

void ResultTest::testOk() {
    auto result = sc::Result<int>::Ok(42);
    QVERIFY(result.isOk());
    QCOMPARE(result.value(), 42);
}

void ResultTest::testErr() {
    auto error = sc::Error(sc::ErrorCode::NotFound, "Not found");
    auto result = sc::Result<int>::Err(error);
    QVERIFY(result.isErr());
    QCOMPARE(result.error().code(), sc::ErrorCode::NotFound);
}

QTEST_APPLESS_MAIN(ResultTest)
#include "result_test.moc"
```

### Mocking

Use dependency injection for mocking:

```cpp
class MockNetworkClient : public sc::INetworkClient {
public:
    MOCK_METHOD(Result<HttpResponse>, get, (const QString& url), (override));
};
```

---

## Test Execution

### CMake Integration

```cmake
enable_testing()

add_executable(result_test
    tests/unit/core/result_test.cpp
)

target_link_libraries(result_test PRIVATE
    Qt6::Test
    soul_core
)

add_test(NAME result_test COMMAND result_test)
```

### Running Tests

```bash
# Build tests
cmake --build build --target all

# Run all tests
cd build
ctest

# Run specific test
ctest -R result_test

# Run with verbose output
ctest -v
```

---

## Test Guidelines

### What to Test

- **Normal paths**: Expected behavior
- **Edge cases**: Boundary conditions
- **Error paths**: Error handling
- **Concurrency**: Thread safety
- **Performance**: Time-sensitive operations

### What Not to Test

- **Third-party code**: Trust Qt and standard library
- **Implementation details**: Test behavior, not implementation
- **UI aesthetics**: Focus on functionality, not visual appearance

### Test Isolation

- Each test should be independent
- Use `init()`/`cleanup()` for setup/teardown
- Avoid shared state between tests

---

## CI/CD Integration

### Continuous Integration

- **Platforms**: Windows, macOS, Linux
- **Triggers**: Pull requests, main branch pushes
- **Steps**:
  1. Build
  2. Run tests
  3. Generate coverage report
  4. Static analysis

### Coverage Report

- **Tool**: gcov / lcov
- **Format**: HTML
- **Threshold**: Fail if coverage drops below target

---

## Testing Review Checklist

- ☑ Unit tests cover normal paths
- ☑ Unit tests cover edge cases
- ☑ Unit tests cover error paths
- ☑ Integration tests cover cross-module workflows
- ☑ UI tests cover component interactions
- ☑ Regression tests cover bug fixes
- ☑ Tests are properly isolated
- ☑ Coverage meets target requirements
- ☑ Tests pass on all platforms
- ☑ CI/CD pipeline runs tests on every PR