#include "soul/di/module.h"

namespace sc {
namespace di {

void Module::initialize() {
}

void Module::shutdown() {
    Container::instance().clear();
}

} // namespace di
} // namespace sc