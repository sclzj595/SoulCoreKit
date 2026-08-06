#ifndef SOUL_CONFIGURATION_MODULE_H
#define SOUL_CONFIGURATION_MODULE_H

#include "soul/core/module.h"

namespace sc {

class ConfigModule : public Module {
public:
    ConfigModule();
    Result<void> init() override;
    void cleanup() override;
};

} // namespace sc

#endif