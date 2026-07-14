# SoulCoreKit 构建系统规范

## 概述

本文档定义了 SoulCoreKit 的 CMake 构建系统配置，包括模块结构、依赖关系和构建选项。

---

## CMake 要求

- **最低版本**: 3.16
- **C++ 标准**: 17
- **Qt 版本**: 6.5+

---

## 项目结构

```
SoulCoreKit/
├── CMakeLists.txt              # 根目录 CMakeLists.txt
├── include/                    # 公共头文件
│   └── soul/
│       ├── core/
│       ├── ui/
│       ├── network/
│       └── ...
├── src/                        # 源文件
│   └── soul/
│       ├── core/
│       ├── ui/
│       ├── network/
│       └── ...
├── tests/                      # 单元测试
├── examples/                   # 示例应用
└── cmake/                      # CMake 模块
    └── FindSoulCoreKit.cmake
```

---

## 根目录 CMakeLists.txt

### 关键设置

```cmake
cmake_minimum_required(VERSION 3.16)
project(SoulCoreKit VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
```

### Qt 配置

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

### 构建选项

| 选项 | 描述 | 默认值 |
|--------|-------------|---------|
| `BUILD_SHARED_LIBS` | 构建共享库 | OFF |
| `BUILD_TESTS` | 构建单元测试 | ON |
| `BUILD_EXAMPLES` | 构建示例应用 | ON |
| `BUILD_DOCS` | 使用 Doxygen 构建文档 | OFF |
| `ENABLE_WARNINGS` | 启用编译器警告 | ON |
| `ENABLE_SANITIZERS` | 启用 sanitizers（仅开发环境） | OFF |

---

## 模块结构

### 每个模块作为静态库

每个模块构建为静态库：

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

### 聚合接口库

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

## 依赖管理

### 模块依赖

| 模块 | 依赖 |
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

## 构建类型

### Debug

- `-DCMAKE_BUILD_TYPE=Debug`
- 启用调试符号
- 无优化
- 可使用 Sanitizers

### Release

- `-DCMAKE_BUILD_TYPE=Release`
- 完全优化
- 无调试符号

### RelWithDebInfo

- `-DCMAKE_BUILD_TYPE=RelWithDebInfo`
- 优化并保留调试符号

---

## 安装

### 安装目标

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

### 包配置

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

## 外部项目使用

### 在其他项目中使用

```cmake
find_package(SoulCoreKit REQUIRED)

add_executable(MyApp main.cpp)
target_link_libraries(MyApp PRIVATE sc::SoulCoreKit)
```

---

## 编译器警告

### 警告标志

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

### 启用 Sanitizers

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

## 构建系统审查检查清单

- ☑ CMakeLists.txt 遵循标准结构
- ☑ Qt6 依赖正确指定
- ☑ 构建选项正确定义
- ☑ 模块依赖正确配置
- ☑ 无循环依赖
- ☑ 定义了安装目标
- ☑ 生成了包配置文件
- ☑ 启用了编译器警告
- ☑ Sanitizers 可供开发使用
- ☑ 示例和测试按需构建
