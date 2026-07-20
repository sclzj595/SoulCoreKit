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
| **Protocol-Agnostic Network** | Unified HTTP/TCP/WebSocket interface with strategy pattern and interceptor chain, using `sc::network` nested namespace |
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
| **soul_network** | Network communication | `HttpClient`, `WebSocket`, `TcpClient`, `NetworkFactory`, policies/interceptors/adapters | 45+ | ~1200 |
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

### Network Module (HTTP Client)

```cpp
#include "soul/network/http_client.h"
#include "soul/network/http_request.h"
#include "soul/network/http_response.h"

// Create HTTP client
sc::network::HttpClient client;
client.setTimeout(30000);
client.setRetryPolicy(sc::network::RetryPolicy(3));

// Add interceptors
client.addInterceptor(std::make_shared<sc::network::LoggingInterceptor>());
client.addInterceptor(std::make_shared<sc::network::AuthInterceptor>());

// Build request
sc::network::HttpRequest request(sc::network::HttpMethod::Get, QUrl("https://api.example.com/users"));
request.addHeader("Accept", "application/json")
       .addParam("page", 1)
       .addParam("limit", 10);

// Send synchronous request
auto result = client.send(request);
if (result.isOk()) {
    auto response = result.unwrap();
    qDebug() << "Status:" << response.statusCode();
    qDebug() << "Body:" << response.text();
} else {
    qDebug() << "Error:" << result.unwrapErr().message();
}

// Send asynchronous request
client.sendAsync(request, [](const sc::Result<sc::network::HttpResponse>& result) {
    if (result.isOk()) {
        auto response = result.unwrap();
        qDebug() << "Async Status:" << response.statusCode();
    }
});
```

### Network Module (WebSocket)

```cpp
#include "soul/network/web_socket.h"
#include "soul/network/policy/reconnect_policy.h"

// Create WebSocket client with reconnect policy
sc::network::WebSocket ws;
ws.setReconnectPolicy(sc::network::ReconnectPolicy(true, std::chrono::seconds(5), 10));

// Set callbacks
ws.setMessageCallback([](const QString& message) {
    qDebug() << "Received:" << message;
});

ws.setConnectedCallback([]() {
    qDebug() << "Connected!";
});

ws.setDisconnectedCallback([]() {
    qDebug() << "Disconnected!";
});

// Connect and send
ws.connectToServer(QUrl("ws://localhost:8080"));
ws.sendTextMessage("Hello World");
```

### Network Module (HTTP API)

```cpp
#include "soul/network/http_api.h"

// Create API client
auto client = std::make_shared<sc::network::HttpClient>();
sc::network::HttpApi<void> api(client);

// Chain API call
api.get("/api/users")
   .param("page", 1)
   .param("limit", 10)
   .header("Authorization", "Bearer token")
   .onSuccess([](const QJsonDocument& data) {
       qDebug() << "Users:" << data.toJson();
   })
   .onFailure([](const sc::Error& error) {
       qDebug() << "API Error:" << error.message().c_str();
   })
   .execute();
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
| `BUILD_SHARED_LIBS` | Build shared libraries | OFF |

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
- Use `sc::network` nested namespace for network module code

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
**Version**: 1.2.0 (Network Module Refactored)  
**Maintainer**: SoulCoreKit Team  
**Contact**: soulcorekit@gmail.com
