#include "soul/core/platform.h"

#include <QOperatingSystemVersion>
#include <string>

#ifdef _WIN32
#include <windows.h>
#elif __APPLE__
#include <sys/utsname.h>
#elif __linux__
#include <sys/utsname.h>
#include <fstream>
#endif

namespace sc {

static RuntimeMode s_runtimeMode = RuntimeMode::Development;

OS Platform::os() {
#ifdef _WIN32
    return OS::Windows;
#elif __APPLE__
    return OS::macOS;
#elif __linux__
    return OS::Linux;
#else
    return OS::Unknown;
#endif
}

std::string Platform::osName() {
    switch (os()) {
    case OS::Windows: return "Windows";
    case OS::macOS: return "macOS";
    case OS::Linux: return "Linux";
    default: return "Unknown";
    }
}

std::string Platform::osVersion() {
#ifdef _WIN32
    const auto version = QOperatingSystemVersion::current();
    if (version.majorVersion() >= 10) {
        if (version.microVersion() >= 22000) return "11";
        return "10";
    }
    return std::to_string(version.majorVersion()) + "." + std::to_string(version.minorVersion());
#elif __APPLE__
    const auto version = QOperatingSystemVersion::current();
    return std::to_string(version.majorVersion()) + "." + std::to_string(version.minorVersion());
#elif __linux__
    utsname info;
    uname(&info);
    return std::string(info.release);
#else
    return "Unknown";
#endif
}

std::string Platform::architecture() {
#ifdef _WIN32
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : "x86";
#elif __APPLE__ || __linux__
    return sizeof(void*) == 8 ? "x64" : "x86";
#else
    return "Unknown";
#endif
}

RuntimeMode Platform::runtimeMode() {
    return s_runtimeMode;
}

void Platform::setRuntimeMode(RuntimeMode mode) {
    s_runtimeMode = mode;
}

bool Platform::isDebugBuild() {
#ifdef _DEBUG
    return true;
#else
    return false;
#endif
}

bool Platform::isReleaseBuild() {
    return !isDebugBuild();
}

std::string Platform::cpuInfo() {
#ifdef _WIN32
    return "Unknown";
#elif __linux__
    std::ifstream file("/proc/cpuinfo");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.substr(0, 10) == "model name") {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    return line.substr(colon + 2);
                }
            }
        }
    }
    return "Unknown";
#else
    return "Unknown";
#endif
}

size_t Platform::memorySize() {
#ifdef _WIN32
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    return status.ullTotalPhys;
#elif __linux__
    std::ifstream file("/proc/meminfo");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.substr(0, 8) == "MemTotal") {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string value = line.substr(colon + 2);
                    return std::stoull(value) * 1024;
                }
            }
        }
    }
    return 0;
#else
    return 0;
#endif
}

}