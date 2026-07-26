#include "soul/event/typed_event_bus_detail.h"
#include "soul/async/thread_pool.h"
#include "soul/logging/logger.h"

namespace sc {
namespace detail {

void dispatchAsync(std::function<void()> task) {
    ThreadPool::instance().start(std::move(task));
}

void logEventBusException(const std::string& message, const std::string& module) {
    Logger::instance().error(message, module);
}

void logEventBusUnknownException(const std::string& module) {
    Logger::instance().error("Unknown exception", module);
}

} // namespace detail
} // namespace sc
