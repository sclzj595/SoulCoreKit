#ifndef SOUL_OBSERVABILITY_MODULE_H
#define SOUL_OBSERVABILITY_MODULE_H

#include "soul/core/module.h"

namespace sc {
namespace observability {

class ObsModule : public Module {
public:
    ObsModule();
    Result<void> init() override;
    void cleanup() override;
};

} // namespace observability
} // namespace sc

#endif