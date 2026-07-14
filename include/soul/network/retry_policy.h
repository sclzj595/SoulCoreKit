#ifndef SOUL_NETWORK_RETRY_POLICY_H
#define SOUL_NETWORK_RETRY_POLICY_H

#include "soul/network/policy/retry_policy.h"

namespace sc {

using RetryStrategy [[deprecated("Use sc::network::RetryStrategy instead")]] = sc::network::RetryStrategy;
using RetryPolicy [[deprecated("Use sc::network::RetryPolicy instead")]] = sc::network::RetryPolicy;

}

#endif