#include "soul/event/subscription.h"

namespace sc {

Subscription::Subscription(const QString& eventType, EventPriority priority, Callback callback)
    : m_eventType(eventType), m_priority(priority), m_callback(callback), m_valid(true) {}

QString Subscription::eventType() const {
    return m_eventType;
}

EventPriority Subscription::priority() const {
    return m_priority;
}

Subscription::Callback Subscription::callback() const {
    return m_callback;
}

bool Subscription::isValid() const {
    return m_valid;
}

void Subscription::invalidate() {
    m_valid = false;
}

}