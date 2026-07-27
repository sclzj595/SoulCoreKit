#ifndef SOUL_EVENT_TYPED_EVENT_BUS_DETAIL_H
#define SOUL_EVENT_TYPED_EVENT_BUS_DETAIL_H

#include <functional>
#include <string>

namespace sc {
namespace detail {

// Internal helpers for TypedEventBus.
// Declared here to avoid leaking `soul/async/thread_pool.h` and
// `soul/logging/logger.h` into every translation unit that includes
// `typed_event_bus.h`. Implementations live in `typed_event_bus_detail.cpp`
// and link against `soul_async` + `soul_logging` privately.

// Dispatches `task` on the global ThreadPool. Equivalent to:
//   ThreadPool::instance().start(std::move(task));
void dispatchAsync(std::function<void()> task);

// Logs an exception with context tag. `module` is typically "EventBus".
void logEventBusException(const std::string& message, const std::string& module);
void logEventBusUnknownException(const std::string& module);

} // namespace detail
} // namespace sc

#endif // SOUL_EVENT_TYPED_EVENT_BUS_DETAIL_H
