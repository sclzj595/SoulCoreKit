#include "soul/plugin/module.h"

namespace sc {
namespace plugin {

void Module::initialize() {
}

void Module::shutdown() {
    PluginManager::instance().shutdownAllPlugins();
}

} // namespace plugin
} // namespace sc