# ============================================================================
# mingw1120_64.cmake — Qt 6.5.3 匹配工具链 (GCC 11.2.0)
# ============================================================================
#
# 为什么必须使用 GCC 11.2.0:
#   Qt 6.5.3 (mingw_64) 官方预编译包使用 MinGW 11.2.0 编译。
#   若使用其他版本的 MinGW (如 GCC 14) 编译本项目, 会因 libstdc++/libgcc
#   的 C++ ABI 版本不匹配, 导致运行时 0xc0000139 (DLL entry point not found)、
#   heap corruption、SEGFAULT 等难以排查的问题。
#
# 本文件固化 Qt 6.5.3 配套工具链路径, 保证构建工具链与 Qt 二进制 ABI 一致。
#
# 使用方式:
#   cmake -S . -B build -G "MinGW Makefiles" `
#         -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw1120_64.cmake
#
# 路径自适应: 若 QT_ROOT 未显式指定, 按常见安装位置探测。

if(NOT DEFINED QT_ROOT)
    # 常见安装位置探测
    foreach(_candidate
            "F:/IDE.2/QT"
            "C:/Qt"
            "D:/Qt"
            "G:/Qt")
        if(EXISTS "${_candidate}/6.5.3/mingw_64/lib/cmake/Qt6/Qt6Config.cmake" AND
           EXISTS "${_candidate}/Tools/mingw1120_64/bin/g++.exe")
            set(QT_ROOT "${_candidate}" CACHE PATH "Qt 6.5.3 安装根目录")
            break()
        endif()
    endforeach()
endif()

if(NOT DEFINED QT_ROOT OR NOT EXISTS "${QT_ROOT}")
    message(FATAL_ERROR
        "未找到 Qt 6.5.3 + MinGW 11.2.0 工具链。"
        "请显式指定: -DQT_ROOT=<Qt安装根目录> "
        "(该目录下需存在 6.5.3/mingw_64 和 Tools/mingw1120_64)")
endif()

# --- 编译器 ---
set(CMAKE_C_COMPILER   "${QT_ROOT}/Tools/mingw1120_64/bin/gcc.exe"   CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "${QT_ROOT}/Tools/mingw1120_64/bin/g++.exe"   CACHE FILEPATH "C++ compiler")
set(CMAKE_RC_COMPILER  "${QT_ROOT}/Tools/mingw1120_64/bin/windres.exe" CACHE FILEPATH "Resource compiler")

# --- Qt 路径 (匹配工具链的 mingw_64) ---
list(APPEND CMAKE_PREFIX_PATH "${QT_ROOT}/6.5.3/mingw_64")

# --- 工具链运行时 DLL 目录 (供 windeployqt 之后补全) ---
set(MINGW1120_BIN_DIR "${QT_ROOT}/Tools/mingw1120_64/bin")

message(STATUS "SoulCoreKit: 使用 Qt 6.5.3 匹配工具链 MinGW 11.2.0")
message(STATUS "  CXX       = ${CMAKE_CXX_COMPILER}")
message(STATUS "  Qt Prefix = ${QT_ROOT}/6.5.3/mingw_64")
