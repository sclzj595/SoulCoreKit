#include <algorithm>
#include <filesystem>
#include <mutex>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// MinGW: GetProcAddress 返回 FARPROC (long long int (*)()),
// reinterpret_cast 到 PluginXxxFunc 触发 -Werror=cast-function-type。
// 这是跨 DLL 边界的标准做法，在所有平台上都是安全的。
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-function-type"
#endif

#include "soul/plugin/plugin_manager.h"
#include "soul/logging/logger.h"

namespace sc {
namespace plugin {

PluginManager::PluginHandle::~PluginHandle()
{
    if (initialized) {
        if (libraryHandle) {
            auto shutdown = reinterpret_cast<PluginShutdownFunc>(
#if defined(_WIN32)
                GetProcAddress(reinterpret_cast<HMODULE>(libraryHandle), "pluginShutdown")
#else
                dlsym(libraryHandle, "pluginShutdown")
#endif
            );
            if (shutdown) {
                shutdown();
            }
        }
        initialized = false;
        enabled = false;
    }
    if (libraryHandle) {
        // [审计] plugin 的删除器可能引用 DLL 内的 pluginDestroy 函数,
        // 必须 reset shared_ptr 之后再 FreeLibrary, 否则在 DLL 卸载后
        // 析构 shared_ptr 时调用已卸载的函数指针 → UAF。
        plugin.reset();
#if defined(_WIN32)
        FreeLibrary(reinterpret_cast<HMODULE>(libraryHandle));
#else
        dlclose(libraryHandle);
#endif
    }
}

bool PluginManager::loadPlugin(const std::string& path)
{
    return loadNativePlugin(path);
}

bool PluginManager::loadNativePlugin(const std::string& path)
{
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
            // [审计] 删除器必须真正释放插件实例, 原空操作删除器导致每次加载永久泄漏。
            // 正确方式: 调用 DLL 导出的 pluginDestroy(跨 CRT 安全); 若未导出, 回退到
            // IPlugin 的虚析构 delete(接口声明了 virtual ~IPlugin(), 亦跨 DLL 安全)。
            auto destroyFunc = reinterpret_cast<PluginDestroyFunc>(
#if defined(_WIN32)
                GetProcAddress(handle, "pluginDestroy")
#else
                dlsym(handle, "pluginDestroy")
#endif
            );
            handlePtr->plugin = std::shared_ptr<IPlugin>(static_cast<IPlugin*>(pluginInstance),
                [destroyFunc](IPlugin* p) {
                    if (destroyFunc) {
                        destroyFunc(p);
                    } else {
                        delete p;
                    }
                });
        }
    }

    m_plugins[pluginId] = std::move(handlePtr);
    return true;
}

bool PluginManager::unloadPlugin(const std::string& pluginId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(pluginId);
    if (it != m_plugins.end()) {
        if (it->second->initialized) {
            shutdownPluginLocked(pluginId);
        }
        m_plugins.erase(it);
        return true;
    }
    return false;
}

bool PluginManager::initializePlugin(const std::string& pluginId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return initializePluginLocked(pluginId);
}

bool PluginManager::initializePluginLocked(const std::string& pluginId)
{
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

bool PluginManager::shutdownPlugin(const std::string& pluginId)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return shutdownPluginLocked(pluginId);
}

bool PluginManager::shutdownPluginLocked(const std::string& pluginId)
{
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

void PluginManager::loadAllPlugins(const std::string& directory)
{
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
    }
    catch (const std::exception& e) {
        Logger::instance().error(std::string("PluginManager::loadAllPlugins failed: ") + e.what());
    }
    catch (...) {
        // Blanket catch: top-level barrier in plugin loading loop.
        // A single plugin failure must not abort loading of remaining plugins.
        Logger::instance().error("PluginManager::loadAllPlugins unknown exception");
    }
}

void PluginManager::initializeAllPlugins()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_plugins) {
        initializePluginLocked(pair.first);
    }
}

void PluginManager::shutdownAllPlugins()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto& pair : m_plugins) {
        shutdownPluginLocked(pair.first);
    }
}

std::shared_ptr<IPlugin> PluginManager::getPlugin(const std::string& pluginId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(pluginId);
    if (it != m_plugins.end()) {
        return it->second->plugin;
    }
    return nullptr;
}

std::vector<std::string> PluginManager::getPluginIds() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<std::string> ids;
    ids.reserve(m_plugins.size());
    for (const auto& pair : m_plugins) {
        ids.push_back(pair.first);
    }
    return ids;
}

std::vector<std::shared_ptr<IPlugin>> PluginManager::getAllPlugins() const
{
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

bool PluginManager::isPluginLoaded(const std::string& pluginId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_plugins.find(pluginId) != m_plugins.end();
}

bool PluginManager::isPluginInitialized(const std::string& pluginId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_plugins.find(pluginId);
    if (it != m_plugins.end()) {
        return it->second->initialized;
    }
    return false;
}

int PluginManager::pluginCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_plugins.size());
}

} // namespace plugin
} // namespace sc

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#endif
