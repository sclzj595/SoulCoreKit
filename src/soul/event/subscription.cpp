#include "soul/event/subscription.h"

namespace sc {

MessageSubscription::MessageSubscription(const QString& eventType, EventPriority priority, Callback callback)
    : m_eventType(eventType), m_priority(priority), m_callback(callback), m_valid(true) {}

QString MessageSubscription::eventType() const {
    return m_eventType;
}

EventPriority MessageSubscription::priority() const {
    return m_priority;
}

MessageSubscription::Callback MessageSubscription::callback() const {
    return m_callback;
}

bool MessageSubscription::isValid() const {
    return m_valid;
}

void MessageSubscription::invalidate() {
    m_valid = false;
}

}