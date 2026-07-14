# SoulCoreKit

> A modern C++ application framework based on Qt6, designed for building high-performance, scalable cross-platform applications.

[![License](https://img.shields.io/github/license/SoulCoreKit/SoulCoreKit.svg)](LICENSE)
[![GitHub release](https://img.shields.io/github/release/SoulCoreKit/SoulCoreKit.svg)](https://github.com/SoulCoreKit/SoulCoreKit/releases)
[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue.svg)](https://github.com/SoulCoreKit/SoulCoreKit)
[![C++ Version](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Qt Version](https://img.shields.io/badge/Qt-6.5%2B-blue.svg)](https://www.qt.io)

---

## Table of Contents

- [Features](#features)
- [Architecture](#architecture)
- [Modules](#modules)
- [Getting Started](#getting-started)
- [Usage Examples](#usage-examples)
- [Build System](#build-system)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [License](#license)

---

## Features

| Feature | Description |
|---------|-------------|
| **Modular Architecture** | 12 independent modules, combine as needed, low coupling and high cohesion |
| **Protocol-Agnostic Network** | Unified HTTP/TCP/WebSocket interface with strategy pattern and interceptor chain |
| **Event-Driven Architecture** | Publish-subscribe based event bus with Qt signal bridging |
| **Async Task Framework** | Thread pool based async execution with coroutine-style programming |
| **Unified Error Handling** | `Result<T>` pattern for type-safe error propagation |
| **Extensible Storage** | Memory, file, SQLite storage backends |
| **Declarative UI** | Modern UI component library with theme switching and glass effect support |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         Application Layer                             │
└─────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          Service Layer                                 │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │ soul_auth│  │ soul_base│  │ soul_ui  │  │ soul_app │  │ soul_... │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘ │
│       │             │             │             │             │        │
└───────┼─────────────┼─────────────┼─────────────┼─────────────┼────────┘
        │             │             │             │             │
        ▼             ▼             ▼             ▼             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          Infrastructure Layer                          │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │soul_netw │  │soul_async│  │soul_event│  │soul_stor │  │soul_log  │ │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘ │
│       │             │             │             │             │        │
└───────┼─────────────┼─────────────┼─────────────┼─────────────┼────────┘
        │             │             │             │             │
        ▼             ▼             ▼             ▼             ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                          Core Layer                                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐ │
│  │ soul_core│  │soul_util │  │soul_conf │  │   Qt6    │  │  C++17   │ │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘  └──────────┘ │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## Modules

| Module | Responsibility | Core Classes | Files | Lines |
|--------|---------------|--------------|-------|-------|
| **soul_core** | Core infrastructure | `Result<T>`, `Error`, `IInterface`, `Singleton`, `Factory<T>` | 11 | ~200 |
| **soul_utils** | Utility library | JSON/File/String/Crypto/Image utilities | 10 | ~500 |
| **soul_logging** | Logging system | `Logger`, `ISink`, `LogFormatter` | 12 | ~300 |
| **soul_configuration** | Configuration management | `IConfiguration`, `JsonConfiguration`, `IniConfiguration` | 5 | ~200 |
| **soul_async** | Async execution | `ThreadPool`, `TaskRunner`, `Future`, `Promise` | 8 | ~300 |
| **soul_event** | Event bus | `EventBus`, `Subscription`, `QtSignalAdapter` | 6 | ~200 |
| **soul_network** | Network communication | `INetwork`, `NetworkFactory`, policies/interceptors/adapters | 22 | ~800 |
| **soul_storage** | Data storage | `IStorage`, `SqliteDatabase`, `Cache`, `Repository` | 9 | ~300 |
| **soul_auth** | Authentication | `AuthManager`, `TokenManager`, `Permission` | 6 | ~200 |
| **soul_ui** | User interface | 30+ UI components, Theme, Style, Animation | 40 | ~1200 |
| **soul_base** | Business base | `BaseService`, `BaseRepository`, `BaseWidget` | 7 | ~200 |

---

## Getting Started

### Prerequisites

- **C++ Compiler**: GCC 8+, Clang 9+, MSVC 2019+
- **Qt**: 6.5+ (Core, Network, WebSockets, Sql, Widgets)
- **CMake**: 3.16+

### Installation

```bash
# Clone the repository
git clone https://github.com/SoulCoreKit/SoulCoreKit.git
cd SoulCoreKit

# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build . --config Release

# Install (optional)
cmake --install .
```

### Integration with Your Project

**CMakeLists.txt**:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Network WebSockets Sql Widgets)

add_subdirectory(SoulCoreKit)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE SoulCoreKit)
```

---

## Usage Examples

### Network Module

```cpp
#include "soul/network/soul_network.h"

// Create network instances for different protocols
auto http = sc::network::NetworkFactory::instance().create(QUrl("http://api.example.com"));
auto tcp = sc::network::NetworkFactory::instance().create(QUrl("tcp://localhost:8080"));
auto ws = sc::network::NetworkFactory::instance().create(QUrl("ws://localhost:8080"));

// Configure interceptors and policies
http->addInterceptor(std::make_shared<sc::network::LoggingInterceptor>());
http->setPolicy(std::make_shared<sc::network::RetryPolicy>(3));

// Send message synchronously
sc::network::NetworkMessage msg;
msg.api = "/api/users";
msg.payload = "{\"name\":\"test\"}";
auto result = http->send(msg);

if (result.isOk()) {
    qDebug() << result.unwrap().payload;
}

// Send message asynchronously
http->sendAsync(msg, [](const sc::Result<sc::network::NetworkMessage>& result) {
    if (result.isOk()) {
        qDebug() << result.unwrap().payload;
    }
});
```

### Error Handling

```cpp
#include "soul/core/result.h"

sc::Result<int> divide(int a, int b) {
    if (b == 0) {
        return sc::Error(1, "Division by zero");
    }
    return a / b;
}

auto result = divide(10, 2);
if (result.isOk()) {
    qDebug() << "Result:" << result.unwrap();
} else {
    qDebug() << "Error:" << result.unwrapErr().message();
}
```

### Event Bus

```cpp
#include "soul/event/event_bus.h"

// Define event
class UserLoggedInEvent : public sc::IEvent {
public:
    std::string userId;
    std::string interfaceName() const override { return "UserLoggedInEvent"; }
};

// Subscribe to event
auto subscription = sc::EventBus::instance().subscribe<UserLoggedInEvent>(
    [](const UserLoggedInEvent& event) {
        qDebug() << "User logged in:" << event.userId.c_str();
    }
);

// Publish event
sc::EventBus::instance().publish(UserLoggedInEvent{"user123"});
```

### Logging

```cpp
#include "soul/logging/logger.h"
#include "soul/logging/console_sink.h"
#include "soul/logging/file_sink.h"

// Initialize logger with multiple sinks
auto consoleSink = std::make_shared<sc::ConsoleSink>();
auto fileSink = std::make_shared<sc::FileSink>("app.log");

sc::Logger::instance().addSink(consoleSink);
sc::Logger::instance().addSink(fileSink);

// Log messages
SC_LOG_TRACE("Trace message");
SC_LOG_DEBUG("Debug message");
SC_LOG_INFO("Info message");
SC_LOG_WARNING("Warning message");
SC_LOG_ERROR("Error message");
SC_LOG_FATAL("Fatal message");
```

---

## Build System

### CMake Options

| Option | Description | Default |
|--------|-------------|---------|
| `CMAKE_BUILD_TYPE` | Build type (Debug/Release) | Release |
| `BUILD_TESTS` | Build test suite | ON |
| `BUILD_EXAMPLES` | Build examples | ON |
| `BUILD_DOCS` | Build documentation | OFF |

### Module Dependencies

```
soul_core
    ├── soul_utils
    ├── soul_logging
    ├── soul_async
    │       ├── soul_configuration
    │       └── soul_event
    │               ├── soul_network
    │                       ├── soul_storage
    │                               ├── soul_auth
    │                                       ├── soul_ui
    │                                               └── soul_base
```

---

## Documentation

- **API Documentation**: Generated with Doxygen
- **Design Documents**: Located in `docs/` (English) and `docs_chinese/` (Chinese)
- **Quick Start**: See `examples/` directory

### Generate Documentation

```bash
cd SoulCoreKit
doxygen Doxyfile
```

---

## Contributing

Contributions are welcome! Please follow these guidelines:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/your-feature`)
3. Commit your changes (`git commit -m 'Add some feature'`)
4. Push to the branch (`git push origin feature/your-feature`)
5. Create a Pull Request

### Code Style

- Follow the [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- Use `clang-format` for automatic formatting
- Write unit tests for new features
- Document public APIs with Doxygen

---

## License

SoulCoreKit is licensed under the [MIT License](LICENSE).

---

## Acknowledgments

- [Qt Framework](https://www.qt.io) - Cross-platform application framework
- [CMake](https://cmake.org) - Build system
- [Doxygen](https://www.doxygen.nl) - Documentation generator

---

**Project**: SoulCoreKit  
**Version**: 1.0.0  
**Maintainer**: SoulCoreKit Team  
**Contact**: 2575060791@qq.com
