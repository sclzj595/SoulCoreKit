#ifndef SOUL_EVENT_AGGREGATE_H
#define SOUL_EVENT_AGGREGATE_H

// soul_event.h — Event 模块聚合头文件
//
// 一行引入事件总线层:IEvent/EventBus/TypedEventBus/Subscription/
// QtSignalAdapter/IMessageBus/EventPriority。
//
// 用法:
//   #include "soul/soul_event.h"

#include "soul/event/i_event.h"
#include "soul/event/event_bus.h"
#include "soul/event/typed_event_bus.h"
#include "soul/event/subscription.h"
#include "soul/event/qt_signal_adapter.h"
#include "soul/event/i_message_bus.h"
#include "soul/event/event_priority.h"

#endif // SOUL_EVENT_AGGREGATE_H
