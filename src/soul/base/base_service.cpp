#include "soul/base/base_service.h"

namespace sc {

BaseService::BaseService(QObject* parent) : BaseObject(parent) {}

bool BaseService::start() {
    m_running = true;
    return true;
}

void BaseService::stop() {
    m_running = false;
}

bool BaseService::isRunning() const {
    return m_running;
}

}