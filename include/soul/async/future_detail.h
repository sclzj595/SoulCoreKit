#ifndef SOUL_ASYNC_FUTURE_DETAIL_H
#define SOUL_ASYNC_FUTURE_DETAIL_H

#include <string>

namespace sc {
namespace detail {

// Internal helper functions for async module error logging.
// Declared here (instead of calling Logger directly in future.h/task_runner.h)
// to avoid leaking `soul/logging/logger.h` into every translation unit that
// includes these headers. Implementations live in `future_detail.cpp` and
// link against `soul_logging` privately.
//
// Context: short string identifying the call site (e.g. "Future::waitForFinished",
// "TaskRunner::runAsync task").
void logAsyncException(const char* context, const std::string& message);
void logAsyncUnknownException(const char* context);

} // namespace detail
} // namespace sc

#endif // SOUL_ASYNC_FUTURE_DETAIL_H
