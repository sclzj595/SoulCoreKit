#ifndef SOUL_PLUGIN_IPLUGIN_H
#define SOUL_PLUGIN_IPLUGIN_H

#include <string>

#include "plugin_global.h"

struct SC_PLUGIN_EXPORT PluginMetadata {
    const char* id;
    const char* name;
    const char* version;
    const char* description;
    const char* author;
    const char* dependencies;
    int abiVersion;
    int apiVersion;
};

#define PLUGIN_ABI_VERSION 1
#define PLUGIN_API_VERSION 1

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*PluginInitializeFunc)(void);
typedef int (*PluginShutdownFunc)(void);
typedef const PluginMetadata* (*PluginGetMetadataFunc)(void);
typedef void* (*PluginCreateFunc)(const char* type);
typedef void (*PluginDestroyFunc)(void* instance);

#ifdef __cplusplus
}
#endif

namespace sc {
namespace plugin {

class SC_PLUGIN_EXPORT IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual const std::string& id() const = 0;
    virtual const std::string& name() const = 0;
    virtual const std::string& version() const = 0;
    virtual const std::string& description() const = 0;
    virtual const std::string& author() const = 0;

    virtual int initialize() = 0;
    virtual int shutdown() = 0;

    virtual bool isInitialized() const = 0;
    virtual bool isEnabled() const = 0;

    virtual void setEnabled(bool enabled) = 0;

    virtual const PluginMetadata* metadata() const = 0;
};

} // namespace plugin
} // namespace sc

#endif