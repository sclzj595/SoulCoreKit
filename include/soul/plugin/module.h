#ifndef SOUL_PLUGIN_MODULE_H
#define SOUL_PLUGIN_MODULE_H

#include "plugin_manager.h"

namespace sc {
namespace plugin {

class SC_PLUGIN_EXPORT Module {
public:
    static void initialize();
    static void shutdown();
};

} // namespace plugin
} // namespace sc

#endif