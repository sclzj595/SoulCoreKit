# SoulCoreKit Build System Specification

## Overview

This document defines the CMake build system configuration for SoulCoreKit, including module structure, dependencies, and build options.

---

## CMake Requirements

- **Minimum Version**: 3.16
- **C++ Standard**: 17
- **Qt Version**: 6.5+

---

## Project Structure

```
SoulCoreKit/
├── CMakeLists.txt              # Root CMakeLists.txt
├── include/                    # Public headers
│   └── soul/
│       ├── core/
│       ├── ui/
│       ├── network/
│       └── ...
├── src/                        # Source files
│   └── soul/
│       ├── core/
│       ├── ui/
│       ├── network/
│       └── ...
├── tests/                      # Unit tests
├── examples/                   # Example applications
└── cmake/                      # CMake modules
    └── FindSoulCoreKit.cmake
```

---

## Root CMakeLists.txt

### Key Settings

```cmake
cmake_minimum_required(VERSION 3.16)
project(SoulCoreKit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
```

### Qt Configuration

```cmake
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Widgets 
    Network 
    WebSockets 
    Sql 
    Xml 
    Gui
)
```

### Build Options

| Option | Description | Default |
|--------|-------------|---------|
| `BUILD_SHARED_LIBS` | Build shared libraries | OFF |
| `BUILD_TESTS` | Build unit tests | ON |
| `BUILD_EXAMPLES` | Build example applications | ON |
| `BUILD_DOCS` | Build documentation with Doxygen | OFF |
| `ENABLE_WARNINGS` | Enable compiler warnings | ON |
| `ENABLE_SANITIZERS` | Enable sanitizers (dev only) | OFF |

---

## Module Structure

### Static Library Per Module

Each module is built as a static library:

```cmake
add_library(soul_core STATIC
    ${SOUL_CORE_HEADERS}
    ${SOUL_CORE_SOURCES}
)

target_include_directories(soul_core PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(soul_core PUBLIC
    Qt6::Core
)
```

### Interface Library for Aggregation

```cmake
add_library(SoulCoreKit INTERFACE)

target_link_libraries(SoulCoreKit INTERFACE
    soul_core
    soul_base
    soul_logging
    soul_network
    soul_utils
    soul_configuration
    soul_storage
    soul_async
    soul_event
    soul_auth
    soul_ui
)
```

---

## Dependency Management

### Module Dependencies

| Module | Dependencies |
|--------|--------------|
| `soul_core` | Qt6::Core |
| `soul_base` | Qt6::Core, Qt6::Widgets, soul_core |
| `soul_logging` | Qt6::Core, soul_core |
| `soul_network` | Qt6::Core, Qt6::Network, Qt6::WebSockets, soul_core, soul_logging |
| `soul_utils` | Qt6::Core, Qt6::Gui, Qt6::Xml, soul_core |
| `soul_configuration` | Qt6::Core, soul_core, soul_utils |
| `soul_storage` | Qt6::Core, Qt6::Sql, soul_core |
| `soul_async` | Qt6::Core, soul_core, soul_logging |
| `soul_event` | Qt6::Core, soul_core |
| `soul_auth` | Qt6::Core, soul_core, soul_network, soul_storage, soul_utils |
| `soul_ui` | Qt6::Core, Qt6::Widgets, soul_core |

---

## Build Types

### Debug

- `-DCMAKE_BUILD_TYPE=Debug`
- Debug symbols enabled
- No optimization
- Sanitizers available

### Release

- `-DCMAKE_BUILD_TYPE=Release`
- Full optimization
- No debug symbols

### RelWithDebInfo

- `-DCMAKE_BUILD_TYPE=RelWithDebInfo`
- Optimization with debug symbols

---

## Installation

### Install Targets

```cmake
install(TARGETS SoulCoreKit
    EXPORT SoulCoreKitTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/soul
    DESTINATION include
)

install(EXPORT SoulCoreKitTargets
    FILE SoulCoreKitTargets.cmake
    NAMESPACE sc::
    DESTINATION lib/cmake/SoulCoreKit
)
```

### Package Configuration

```cmake
include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/SoulCoreKitConfig.cmake.in
    ${PROJECT_BINARY_DIR}/SoulCoreKitConfig.cmake
    INSTALL_DESTINATION lib/cmake/SoulCoreKit
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/SoulCoreKitConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
```

---

## External Projects

### Usage in Other Projects

```cmake
find_package(SoulCoreKit REQUIRED)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE sc::SoulCoreKit)
```

---

## Compiler Warnings

### Warning Flags

```cmake
if(ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(soul_core PRIVATE
            /W4 /WX
        )
    else()
        target_compile_options(soul_core PRIVATE
            -Wall -Wextra -Wpedantic -Werror
        )
    endif()
endif()
```

---

## Sanitizers

### Enable Sanitizers

```cmake
if(ENABLE_SANITIZERS)
    target_compile_options(soul_core PRIVATE
        -fsanitize=address,undefined
    )
    target_link_options(soul_core PRIVATE
        -fsanitize=address,undefined
    )
endif()
```

---

## Build System Review Checklist

- ☑ CMakeLists.txt follows standard structure
- ☑ Qt6 dependencies correctly specified
- ☑ Build options properly defined
- ☑ Module dependencies correctly configured
- ☑ No circular dependencies
- ☑ Installation targets defined
- ☑ Package configuration files generated
- ☑ Compiler warnings enabled
- ☑ Sanitizers available for development
- ☑ Examples and tests conditionally built