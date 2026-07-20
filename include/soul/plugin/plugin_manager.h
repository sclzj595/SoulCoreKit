#ifndef SOUL_PLUGIN_PLUGIN_MANAGER_H
#define SOUL_PLUGIN_PLUGIN_MANAGER_H

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "../core/singleton.h"
#include "iplugin.h"

namespace sc {
namespace plugin {

class SC_PLUGIN_EXPORT PluginManager : public Singleton<PluginManager> {
    friend class Singleton<PluginManager>;
public:
    struct PluginHandle {
        void* libraryHandle = nullptr;
        std::shared_ptr<IPlugin> plugin;
        PluginMetadata metadata{};
        bool initialized = false;
        bool enabled = false;

        ~PluginHandle() {
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
#if defined(_WIN32)
                FreeLibrary(reinterpret_cast<HMODULE>(libraryHandle));
#else
                dlclose(libraryHandle);
#endif
            }
        }
    };

    bool loadPlugin(const std::string& path);
    bool unloadPlugin(const std::string& pluginId);
    bool initializePlugin(const std::string& pluginId);
    bool shutdownPlugin(const std::string& pluginId);

    void loadAllPlugins(const std::string& directory);
    void initializeAllPlugins();
    void shutdownAllPlugins();

    std::shared_ptr<IPlugin> getPlugin(const std::string& pluginId) const;
    std::vector<std::string> getPluginIds() const;
    std::vector<std::shared_ptr<IPlugin>> getAllPlugins() const;

    bool isPluginLoaded(const std::string& pluginId) const;
    bool isPluginInitialized(const std::string& pluginId) const;

    int pluginCount() const;

private:
    PluginManager() = default;
    ~PluginManager() = default;

    bool loadNativePlugin(const std::string& path);
    bool initializePluginLocked(const std::string& pluginId);
    bool shutdownPluginLocked(const std::string& pluginId);

    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::unique_ptr<PluginHandle>> m_plugins;
};

} // namespace plugin
} // namespace sc

#endif