#include "soul/async/future_detail.h"
#include "soul/logging/logger.h"

namespace sc {
namespace detail {

void logAsyncException(const char* context, const std::string& message) {
    Logger::instance().error(std::string(context) + " exception: " + message);
}

void logAsyncUnknownException(const char* context) {
    Logger::instance().error(std::string(context) + " unknown exception");
}

} // namespace detail
} // namespace sc
