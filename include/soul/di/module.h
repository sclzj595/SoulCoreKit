#ifndef SOUL_DI_MODULE_H
#define SOUL_DI_MODULE_H

#include <memory>

#include "../core/singleton.h"
#include "container.h"

namespace sc {
namespace di {

class SC_DI_EXPORT Module {
public:
    static void initialize();
    static void shutdown();
};

template<typename T>
std::shared_ptr<T> wrapSingleton() {
    return std::shared_ptr<T>(&Singleton<T>::instance(), [](T*) {});
}

template<typename T>
void registerSingleton() {
    Container::instance().bindInstance(&Singleton<T>::instance());
}

} // namespace di
} // namespace sc

#endif