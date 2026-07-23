#include "soul/core/environment.h"

#include <cstdlib>
#include <filesystem>
#include <cstring>
#include <string>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <QByteArray>        // QByteArray（qgetenv 返回类型，避免 incomplete type）
#include <QCoreApplication>  // qgetenv 声明所在头文件
#include <QtCore/qglobal.h>  // qgetenv

namespace sc {

std::string Environment::get(const std::string& key, const std::string& defaultValue) {
    // 使用 Qt 的 qgetenv 跨平台获取环境变量，规避 MSVC C4996 警告
    QByteArray value = qgetenv(key.c_str());
    return value.isNull() ? defaultValue : std::string(value.constData(), static_cast<size_t>(value.size()));
}

void Environment::set(const std::string& key, const std::string& value) {
#ifdef _WIN32
    _putenv_s(key.c_str(), value.c_str());
#else
    setenv(key.c_str(), value.c_str(), 1);
#endif
}

bool Environment::contains(const std::string& key) {
    return !qgetenv(key.c_str()).isNull();
}

std::string Environment::getExecutablePath() {
#ifdef _WIN32
    char path[MAX_PATH];
    GetModuleFileNameA(nullptr, path, MAX_PATH);
    return std::string(path);
#else
    char path[4096];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path));
    return len > 0 ? std::string(path, len) : "";
#endif
}

std::string Environment::getWorkingDirectory() {
    std::filesystem::path cwd = std::filesystem::current_path();
    return cwd.string();
}

void Environment::setWorkingDirectory(const std::string& path) {
    std::filesystem::current_path(path);
}

std::string Environment::getHomeDirectory() {
#ifdef _WIN32
    return get("USERPROFILE", get("HOMEDRIVE") + get("HOMEPATH"));
#else
    return get("HOME");
#endif
}

std::string Environment::getAppDataDirectory() {
#ifdef _WIN32
    return get("APPDATA");
#else
    return getHomeDirectory() + "/.config";
#endif
}

std::string Environment::getTempDirectory() {
#ifdef _WIN32
    return get("TEMP");
#else
    return "/tmp";
#endif
}

std::string Environment::getEnv() {
#ifdef _WIN32
    return get("ENV", "development");
#else
    return get("ENV", "development");
#endif
}

std::unordered_map<std::string, std::string> Environment::parseCommandLine(int argc, char* argv[]) {
    std::unordered_map<std::string, std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.size() > 2 && arg.substr(0, 2) == "--") {
            size_t eq = arg.find('=');
            if (eq != std::string::npos) {
                std::string key = arg.substr(2, eq - 2);
                std::string value = arg.substr(eq + 1);
                args[key] = value;
            } else {
                args[arg.substr(2)] = "true";
            }
        } else if (arg.size() > 1 && arg[0] == '-') {
            std::string key = arg.substr(1);
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args[key] = argv[++i];
            } else {
                args[key] = "true";
            }
        }
    }
    return args;
}

}