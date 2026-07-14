#ifndef SOUL_EVENT_SUBSCRIPTION_H
#define SOUL_EVENT_SUBSCRIPTION_H

#include <QString>
#include <memory>
#include "event_bus.h"
#include "event_priority.h"

namespace sc {

class Subscription;
using SubscriptionPtr = std::shared_ptr<Subscription>;

class MessageSubscription {
public:
    using Callback = std::function<void(const std::shared_ptr<void>&)>;

    MessageSubscription(const QString& eventType, EventPriority priority, Callback callback);

    QString eventType() const;
    EventPriority priority() const;
    Callback callback() const;
    bool isValid() const;
    void invalidate();

private:
    QString m_eventType;
    EventPriority m_priority;
    Callback m_callback;
    bool m_valid;
};

using MessageSubscriptionPtr = std::shared_ptr<MessageSubscription>;

}

#endif