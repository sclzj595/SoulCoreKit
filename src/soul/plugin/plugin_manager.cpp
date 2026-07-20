#include <algorithm>
#include <filesystem>

#include "soul/plugin/plugin_manager.h"

namespace sc {
namespace plugin {

bool PluginManager::loadPlugin(const std::string& path) {
    return loadNativePlugin(path);
}

bool PluginManager::loadNativePlugin(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);

#if defined(_WIN32)
    HMODULE handle = LoadLibraryA(path.c_str());
    if (!handle) {
        return false;
    }
#else
    void* handle = dlopen(path.c_str(), RTLD_LAZY);
    if (!handle) {
        return false;
    }
#endif

    auto getMetadata = reinterpret_cast<PluginGetMetadataFunc>(
#if defined(_WIN32)
        GetProcAddress(handle, "pluginGetMetadata")
#else
        dlsym(handle, "pluginGetMetadata")
#endif
    );

    if (!getMetadata) {
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    const PluginMetadata* metadata = getMetadata();
    if (!metadata || !metadata->id) {
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    // 问题5修复：ABI 版本兼容性检查
    if (metadata->abiVersion != PLUGIN_ABI_VERSION) {
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    std::string pluginId(metadata->id);

    auto existing = m_plugins.find(pluginId);
    if (existing != m_plugins.end()) {
#if defined(_WIN32)
        FreeLibrary(handle);
#else
        dlclose(handle);
#endif
        return false;
    }

    auto handlePtr = std::make_unique<PluginHandle>();
    handlePtr->libraryHandle = handle;
    handlePtr->metadata = *metadata;
    handlePtr->initialized = false;
    handlePtr->enabled = false;

    // 问题4修复：如果插件提供了 pluginCreate 函数，创建 IPlugin 实例
    auto createFunc = reinterpret_cast<PluginCreateFunc>(
#if defined(_WIN32)
        GetProcAddress(handle, "pluginCreate")
#else
        dlsym(handle, "pluginCreate")
#endif
    );

    if (createFunc) {
        void* pluginInstance = createFunc("IPlugin");
        if (pluginInstance) {
            // 尝试将 void* 转换为 IPlugin*
            // 注意：这要求插件返回的对象实现了 IPlugin 接口
            handlePtr->plugin = std::shared_ptr<IPlugin>(static_cast<IPlugin*>(pluginInstance),
                [](IPlugin* p) {
                    // 不在这里删除，由插件的 pluginDestroy 函数管理
                });
        }
    }

    m_plugins[pluginId] = std::move(handlePtr);
    return true;
}

bool PluginManager::unloadPlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(pluginId);
    if (it != m_plugins.end()) {
        // 问题6修复：unload 前确保插件已 shutdown
        if (it->second->initialized) {
            shutdownPluginLocked(pluginId);
        }
        m_plugins.erase(it);
        return true;
    }
    return false;
}

// 问题3修复：拆分为加锁的公共方法和不加锁的内部方法
bool PluginManager::initializePlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return initializePluginLocked(pluginId);
}

bool PluginManager::initializePluginLocked(const std::string& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) {
        return false;
    }

    auto& handle = it->second;
    if (handle->initialized) {
        return true;
    }

    void* libHandle = handle->libraryHandle;
    auto initialize = reinterpret_cast<PluginInitializeFunc>(
#if defined(_WIN32)
        GetProcAddress(reinterpret_cast<HMODULE>(libHandle), "pluginInitialize")
#else
        dlsym(libHandle, "pluginInitialize")
#endif
    );

    if (!initialize) {
        return false;
    }

    int result = initialize();
    if (result != 0) {
        return false;
    }

    handle->initialized = true;
    handle->enabled = true;
    return true;
}

bool PluginManager::shutdownPlugin(const std::string& pluginId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return shutdownPluginLocked(pluginId);
}

bool PluginManager::shutdownPluginLocked(const std::string& pluginId) {
    auto it = m_plugins.find(pluginId);
    if (it == m_plugins.end()) {
        return false;
    }

    auto& handle = it->second;
    if (!handle->initialized) {
        return true;
    }

    void* libHandle = handle->libraryHandle;
    auto shutdown = reinterpret_cast<PluginShutdownFunc>(
#if defined(_WIN32)
        GetProcAddress(reinterpret_cast<HMODULE>(libHandle), "pluginShutdown")
#else
        dlsym(libHandle, "pluginShutdown")
#endif
    );

    if (shutdown) {
        shutdown();
    }

    handle->initialized = false;
    handle->enabled = false;
    return true;
}

void PluginManager::loadAllPlugins(const std::string& directory) {
    try {
        for (const auto& entry : std::filesystem::directory_iterator(directory)) {
            if (!entry.is_regular_file()) {
                continue;
            }

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

#if defined(_WIN32)
            if (ext != ".dll") {
                continue;
            }
#elif defined(__APPLE__)
            if (ext != ".dylib") {
                continue;
            }
#else
            if (ext != ".so") {
                continue;
            }
#endif

            loadPlugin(entry.path().string());
        }
    } catch (...) {
    }
}

void PluginManager::initializeAllPlugins() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_plugins) {
        // 问题3修复：调用不加锁的内部方法，避免死锁
        initializePluginLocked(pair.first);
    }
}

void PluginManager::shutdownAllPlugins() {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_plugins) {
        // 问题3修复：调用不加锁的内部方法，避免死锁
        shutdownPluginLocked(pair.first);
    }
}

std::shared_ptr<IPlugin> PluginManager::getPlugin(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(pluginId);
    if (it != m_plugins.end()) {
        return it->second->plugin;
    }
    return nullptr;
}

std::vector<std::string> PluginManager::getPluginIds() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    ids.reserve(m_plugins.size());
    for (const auto& pair : m_plugins) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::shared_ptr<IPlugin>> PluginManager::getAllPlugins() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::shared_ptr<IPlugin>> plugins;
    plugins.reserve(m_plugins.size());
    for (const auto& pair : m_plugins) {
        if (pair.second->plugin) {
            plugins.push_back(pair.second->plugin);
        }
    }
    return plugins;
}

bool PluginManager::isPluginLoaded(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_plugins.find(pluginId) != m_plugins.end();
}

bool PluginManager::isPluginInitialized(const std::string& pluginId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(pluginId);
    if (it != m_plugins.end()) {
        return it->second->initialized;
    }
    return false;
}

int PluginManager::pluginCount() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_plugins.size());
}

} // namespace plugin
} // namespace sc
