# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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