#ifndef SOUL_ORM_MODULE_H
#define SOUL_ORM_MODULE_H

#include "soul/core/module.h"

namespace sc {
namespace orm {

class ORMModule : public Module {
public:
    ORMModule();
    Result<void> init() override;
    void cleanup() override;
};

} // namespace orm
} // namespace sc

#endif