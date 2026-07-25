# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.6.0] - 2026-07-25

### Added

- **SoulRPC Framework**: Complete RPC framework for distributed service communication
  - `ISerializer` abstraction with `JsonSerializer` implementation (JSON serialization/deserialization with type-tagged variant support)
  - `IRpcTransport` transport abstraction with `HttpTransport` implementation (HTTP-based RPC using QNetworkAccessManager)
  - `ServiceDispatcher` server-side dispatcher (thread-safe service registration and dispatch)
  - `ClientProxy` client-side proxy (synchronous RPC calls with automatic UUID request ID generation)
  - `IServiceRegistry` / `InMemoryServiceRegistry` service discovery (instance registration and lookup)
  - `LoadBalancer` with round-robin and random selection strategies
  - Full test suite: serialization, dispatch, proxy, registry, load balancing, value types

- **CI/CD Pipeline**: Automated build, test, lint, and release workflows
  - `.github/workflows/ci.yml` — Multi-platform CI (Ubuntu/Windows), build + test + lint + coverage
  - `.github/workflows/lint.yml` — Clang-Tidy + CppCheck static analysis pipeline
  - `.github/workflows/release.yml` — Multi-platform release pipeline on version tags

- **CMake Presets**: `CMakePresets.json` with `default`, `test`, `lint`, `release` configurations
- **Clang-Tidy Configuration**: `.clang-tidy` with LLVM style + modernize/readability/cppcoreguidelines checks
- **Architecture Decision Records**: 5 ADRs documenting key design decisions
  - ADR-001: Error Handling Boundary Rules (bool vs Result<T>)
  - ADR-002: Module Dependency Rules (5-layer architecture)
  - ADR-003: Memory Management Policy (smart pointers + Qt parent-child)
  - ADR-004: ORM Multi-Database Architecture (Strategy pattern)
  - ADR-005: Thread Safety Policy (4-level classification)

### Changed

- **Error Handling Overhaul**: Replaced all 19 blanket `catch (...)` blocks with specific `catch (const std::exception&)` + fallback, preserving diagnostic information
- **Raw Pointer Cleanup**: Replaced 6 raw `new` allocations without parent with `std::unique_ptr` using Qt `deleteLater` deleters
- **Result<T> Adoption**: Converted 5 public APIs from `bool` to `Result<void>` in AuthManager and TaskRunner
- **CMake Integration**: Added `soul_rpc` library with 6 source files, 7 headers, Qt6::Network dependency

### Fixed

- **CRITICAL**: `buildUpdateSql` SET clause placeholders conflicted with WHERE clause for PostgreSQL — fixed with `startIndex` parameter
- **CRITICAL**: `getUpdateBindValues()` missing — added method returning SET + update_time + WHERE values in correct order
- **CRITICAL**: `buildValueClause` placeholder index not cumulative — fixed with `int& index` reference parameter
- **CRITICAL**: `cleanupWidgetAnimations` called `widget->setGraphicsEffect(nullptr)` in `destroyed` handler (UB) — removed widget method call
- **CRITICAL**: `buildUpdateSql` hardcoded `?` and `deleted=0` — now uses `placeholder()` and `SoftDeleteConfig`
- **MAJOR**: 19 `catch (...)` blocks now preserve exception diagnostic info
- **MAJOR**: 6 raw `new` allocations now use RAII with smart pointers
- **MAJOR**: 5 `bool`-returning public APIs now return `Result<void>`

### Removed

- Zero: No features removed. All changes are additive or bug-fix.

## [1.5.1] - 2026-07-25

### Architecture

- **ORM Multi-Database Refactor (MyBatis-Plus Pattern)**: Transformed `SQLiteRepository` into a generic `SqlRepository<T>` with `ISqlDialect` injection, enabling any database to be supported via dialect injection rather than subclass duplication.
  - `ISqlDialect` abstraction: `SQLiteDialect`, `MySqlDialect`, `PostgreSqlDialect` implementations covering placeholder conversion, identifier escaping, LIMIT/OFFSET syntax, type casting, and soft-delete configuration
  - `SqlRepository<T>`: Single repository implementation that works with any SQL dialect via constructor injection
  - `SoftDeleteConfig`: Configurable soft-delete column name and logic values (default: `deleted=0/1`), with `enabled=false` falling back to physical DELETE
  - `BaseRepository<T>`: Default implementations for `findById`, `findAll`, `removeById`, `count()`, `existsById`, `saveBatch`, `findOne` — subclasses only implement 5 core methods (`find`, `save`, `remove`, `count(query)`, `executeSql`)
  - `QueryWrapper`: Dialect-aware SQL generation for LIMIT/OFFSET, placeholders, and soft-delete predicates
  - `SQLiteRepository<T>` preserved as a `typedef` alias for backward compatibility

### Added

- `src/soul/orm/sql_dialect.cpp`: Full implementation of `ISqlDialect::create()` factory with SQLite/MySQL/PostgreSQL dialect classes
- `SoftDeleteConfig` struct in `sql_dialect.h` for configurable soft-delete behavior
- 3 dialect verification tests: PostgreSQL `$1/$2` placeholders, MySQL `LIMIT offset,count` syntax, SQLite `LIMIT..OFFSET` syntax
- 3 QueryWrapper grouping tests: `and_()` combination, `or_()` grouping semantics, mixed AND/OR precedence

### Changed

- `BaseRepository<T>`: Converted `findById`/`findAll`/`removeById`/`count()`/`existsById`/`saveBatch` from pure virtual to default implementations
- `SqlRepository<T>`: Removed 4 redundant method overrides (`findById`, `findAll`, `removeById`, `count()`), delegating to `BaseRepository` defaults
- `generateInsertSql`/`generateUpdateSql`: Replaced hardcoded `?` placeholders with `m_dialect->convertPlaceholder(index)` for PostgreSQL `$1,$2,...` support
- `buildSelectSql`/`buildCountSql`/`buildDeleteSql`: Replaced hardcoded `deleted=0`/`deleted=1` with `SoftDeleteConfig` from dialect
- `QueryWrapper::Condition`: Replaced `isGroupStart`/`isGroupEnd` booleans with `openParens`/`closeParens` counters for correct nested grouping
- `ThreadPool`: Changed `m_threadPool` from `unique_ptr` to `shared_ptr` for thread-safe lifetime management; all methods copy the pointer before use
- `.gitignore`: Added `/*.py`, `/*.ps1`, `/*.bat`, `/*.sh` rules to prevent root-level temp script commits

### Fixed

- **CRITICAL**: `saveBatch` double-processed new entities (set ID + `beforeInsert` then called `save` which routed to UPDATE) — now delegates directly to `save()`
- **CRITICAL**: `QueryWrapper::or_()` set all sub-conditions to OR logic, producing `OR a OR b` instead of `OR (a AND b)` — now only the first condition is marked OR, rest stay AND
- **CRITICAL**: Mixed AND/OR conditions produced incorrect SQL precedence — `hasOr` detection now wraps all conditions in parentheses
- **CRITICAL**: Nested `or_()` caused unbalanced parentheses due to `singleCondGroup` suppression — `openParens`/`closeParens` accumulators handle arbitrary nesting depth
- **CRITICAL**: `ISqlDialect::create()` was declared but never implemented (linker error) — full factory implementation added
- **CRITICAL**: `INSERT`/`UPDATE` SQL hardcoded `?` placeholder, breaking PostgreSQL `$1,$2` — now uses `convertPlaceholder()`
- **MAJOR**: `ThreadPool::shutdown()` held mutex during `waitForDone()`, risking self-deadlock — now copies `shared_ptr` under lock, calls `waitForDone()` outside lock
- **MAJOR**: `DI Container` Scoped instances used `shared_ptr` with default deleter, causing double-free in `disposeScope()` — Scoped instances now use empty deleter `[](T*){}`
- **MAJOR**: `GlassWidget` leaked `QGraphicsBlurEffect` via raw `new` — wrapped in `std::unique_ptr`
- **MAJOR**: `MemoryRepository` lacked mutex protection on all CRUD operations — added `std::mutex` + `std::lock_guard`
- **MAJOR**: `Singleton` base destructor was non-virtual — made `virtual` to prevent UB on subclass deletion
- **MAJOR**: `DbConnectionPool::release()` didn't decrement `m_totalCount` for disconnected drivers — fixed resource tracking
- **MAJOR**: `testRemove` expected `findById` to return soft-deleted record, but `WHERE deleted=0` filters it — corrected assertion to expect `NotFound` error

### Removed

- 53 temporary development scripts (Python/PowerShell) from repository root

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