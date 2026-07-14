#include "soul/base/base_manager.h"

namespace sc {

BaseManager::BaseManager(QObject* parent) : BaseObject(parent) {}

bool BaseManager::init() {
    m_initialized = true;
    return true;
}

void BaseManager::shutdown() {
    m_initialized = false;
}

bool BaseManager::isInitialized() const {
    return m_initialized;
}

}