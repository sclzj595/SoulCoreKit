# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.5.0] - 2026-07-24

### Added

- **Data Module Implementation**: Complete data layer with multiple database driver support
  - `DatabaseDriverFactory` with SQLite, MySQL, PostgreSQL implementations
  - `MemoryRepository<T>` - Generic in-memory repository implementation
  - `Transaction` and `ITransactionManager` interfaces
  - `DbConnectionPool` - Database connection pooling with acquire/release lifecycle
- **DI Container Enhancement**: `resolve<T>()` now returns `Result<std::shared_ptr<T>>`
  - Proper error handling for unregistered types, null creators, and invalid lifetimes
  - `SingletonWrapper<T>::get()` updated to return `Result`
- **ThreadPool Lifecycle Management**: `init()`, `shutdown()`, `isInitialized()` methods
  - Lazy initialization with double-checked locking
  - Automatic cleanup on application shutdown
- **Test Suite Expansion**: Added comprehensive test coverage
  - `test_data.cpp`: MemoryRepository CRUD, DatabaseDriverFactory, DbConnectionPool (22 tests)
  - `test_orm.cpp`: QueryWrapper, Entity CRTP, SQLiteRepository CRUD (30 tests)
  - `test_mq.cpp`: Message structure, MQFactory, Interface validation (14 tests)
  - `test_ui.cpp`: Theme management, Style, BaseWidget (14 tests)

### Changed

- **ConnectionPool Naming Conflict Resolution**: Renamed `sc::data::ConnectionPool` to `sc::data::DbConnectionPool`
- **Static Library Export Macros**: Fixed `SC_DI_EXPORT` and `SC_PLUGIN_EXPORT` macros for static lib builds
  - Added `SC_DI_STATIC_LIB` and `SC_PLUGIN_STATIC_LIB` conditions to avoid dllimport in static builds
- **Version Bump**: Updated CMakeLists.txt from 1.3.0 to 1.5.0
- **README.md**: Updated version references from 1.3.0 to 1.5.0

### Fixed

- **Data Module Dependencies**: Added Qt6::Sql dependency to `soul_data` module
- **QSqlRecord Include**: Added missing `#include <QSqlRecord>` in database_driver.cpp
- **Transaction State Tracking**: Replaced non-existent `QSqlDatabase::isTransactionActive()` with manual flag tracking
- **ORM Update Bug**: Fixed `SQLiteRepository::updateInternal()` missing `update_time` parameter binding
- **Plugin Cast Warning**: Suppressed `-Wcast-function-type` for dynamic plugin loading on GCC
- **DI Test State Leakage**: Fixed `testIsRegistered` by clearing container state before assertion

## [1.4.0] - 2026-07-23

### Added

- **Missing Implementation Files**:
  - `database_driver.cpp` - DatabaseDriverFactory implementation
  - `memory_repository.cpp` - MemoryRepository compilation unit
  - `transaction.cpp` - Transaction compilation unit

### Changed

- **CMakeLists.txt**: Added `SOUL_DATA_SOURCES` and linked Qt6::Sql for data module

## [1.3.0] - 2026-07-21

### Added

- **Dependency Injection Container**: `sc::di::Container` with factory function registration pattern
  - Lifetime management: `Transient`, `Singleton`, `Scoped`
  - Thread-safe `resolve()` using Double-Checked Locking Pattern (DCLP) with `std::recursive_mutex`
  - `bind<T>()`, `bindSingleton<T>()`, `bindInstance<T>()` registration APIs
  - `SingletonWrapper<T>` for bridging existing `Singleton<T>` instances
  - `registerSingleton<T>()` for four-phase migration strategy
- **Plugin System**: `sc::plugin::PluginManager` with C-ABI boundary interface
  - Cross-platform dynamic library loading (DLL/SO/DYLIB)
  - `PluginMetadata` specification with ABI/API version compatibility checking
  - `IPlugin` interface with lifecycle management (load → initialize → shutdown → unload)
  - `PluginHandle` with automatic shutdown on destruction
  - Thread-safe plugin operations with deadlock-free `initializeAllPlugins()`/`shutdownAllPlugins()`
- **DI Module**: `soul_di` static library with `SC_DI_EXPORT` macro
- **Plugin Module**: `soul_plugin` static library with `SC_PLUGIN_EXPORT` macro
- **Test Suite**: `test_di.cpp` covering DI-T01 through DI-T11 acceptance criteria

### Changed

- Updated CMakeLists.txt to include `soul_di` and `soul_plugin` modules
- Updated `SoulCoreKit` interface library to link new modules
- Updated install targets to include new modules

### Fixed

- DI container: Singleton shared_ptr deleter design (use-after-free)
- DI container: `resolve()` deadlock with recursive dependency resolution
- DI container: DCLP properly implemented with atomic flag
- Plugin system: `initializeAllPlugins()`/`shutdownAllPlugins()` deadlock
- Plugin system: `getPlugin()` always returning nullptr
- Plugin system: Missing ABI version compatibility check
- Plugin system: `PluginHandle` destructor not ensuring plugin shutdown

## [1.2.0] - 2026-07-20

### Added

- **Network Module Fixes**: Cross-module header inclusion protection
  - Added `#ifndef Q_MOC_RUN` guards for `soul/core/*` includes in network headers
  - Ensured all `SC_NETWORK_EXPORT` classes include `network_global.h`

### Fixed

- MOC preprocessor errors when processing non-Qt class headers
- Missing `network_global.h` includes in multiple network headers:
  `monitor.h`, `reconnect_policy.h`, `retry_policy.h`, `timeout_policy.h`,
  `logging_interceptor.h`, `auth_interceptor.h`, `json_codec.h`,
  `http_client_adapter.h`, `tcp_client_adapter.h`, `ws_client_adapter.h`,
  `mqtt_client_adapter.h`, `bluetooth_client_adapter.h`,
  `serial_port_adapter.h`, `named_pipe_adapter.h`, `http_api.h`
- `HttpApi` class namespace moved from `sc` to `sc::network`
- CI build failures on Ubuntu, macOS, and Windows platforms

## [1.1.0] - 2026-07-14

### Added

- **Protocol-Agnostic Network Layer**: Unified `INetwork` interface supporting HTTP/TCP/WebSocket
- **Network Factory**: `NetworkFactory` for protocol-based instance creation
- **Adapter Pattern**: `HttpClientAdapter`, `TcpClientAdapter`, `WsClientAdapter`
- **Policy Layer**: `RetryPolicy`, `TimeoutPolicy`, `HeartbeatPolicy`
- **Interceptor Chain**: `LoggingInterceptor`, `AuthInterceptor`
- **Codec Layer**: `JsonCodec`, `CodecFactory`
- **Monitor Layer**: `Metrics`, `Monitor` for QPS/RT/成功率 statistics
- **Connection Pool**: `ConnectionPool` with max connections and idle timeout
- **Future Protocol Support**: MQTT, Bluetooth, SerialPort, NamedPipe adapters
- **Result<T> Pattern**: Type-safe error handling
- **Event Bus**: Publish-subscribe event system with Qt signal bridging
- **Async Task Framework**: Thread pool based async execution
- **UI Component Library**: 30+ modern UI components with theme support
- **Configuration Management**: JSON/INI configuration with schema validation
- **Storage Layer**: Memory, file, SQLite storage backends
- **Authentication**: AuthManager, TokenManager, Permission system

### Changed

- Refactored network module to support protocol-agnostic abstraction
- Separated interface (`INetwork`) from signal base (`NetworkBase`) to comply with Qt MOC constraints
- Migrated `RetryPolicy` from network root to `policy/` subdirectory
- Extended `IInterceptor` to support all protocols via `NetworkMessage`

### Deprecated

- `sc::IInterceptor` - Use `sc::network::IInterceptor` instead
- `sc::RetryPolicy` - Use `sc::network::RetryPolicy` instead

### Removed

- Redundant `network.h` header file

### Fixed

- `MetricData::minResponseTime` initialization to `INT64_MAX` for correct statistics
- `HeartbeatPolicy::apply()` empty implementation
- `ConnectionPool` missing `QObject` inheritance for QTimer event loop

## [1.0.0] - 2026-07-14

### Added

- Initial release of SoulCoreKit framework
- Core modules: soul_core, soul_utils, soul_logging
- Network module: HTTP, TCP, WebSocket support
- UI module: Modern Qt Widgets component library
- Async module: Thread pool and task system
- Event module: Event bus with Qt integration
- Storage module: SQLite and memory storage
- Configuration module: JSON/INI config support
- Auth module: Token-based authentication
- Base module: Business base classes