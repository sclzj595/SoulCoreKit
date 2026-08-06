#ifndef SOUL_RPC_MODULE_H
#define SOUL_RPC_MODULE_H

#include "soul/core/module.h"

namespace sc {
namespace rpc {

class RpcModule : public Module {
public:
    RpcModule();
    Result<void> init() override;
    void cleanup() override;
};

} // namespace rpc
} // namespace sc

#endif