#ifndef SOUL_SCHEDULER_MODULE_H
#define SOUL_SCHEDULER_MODULE_H

#include "soul/core/module.h"

namespace sc {

class SchedulerModule : public Module {
public:
    SchedulerModule();
    Result<void> init() override;
    Result<void> onStart() override;
    void onStop() override;
    void cleanup() override;
};

} // namespace sc

#endif // SOUL_SCHEDULER_MODULE_H