#ifndef SOUL_EVENT_EVENT_PRIORITY_H
#define SOUL_EVENT_EVENT_PRIORITY_H

namespace sc {

/**
 * @enum EventPriority
 * @brief 事件优先级枚举
 *
 * 优先级从低到高依次为：Low < Normal < High < Critical
 * 高优先级的订阅者会优先收到消息。
 */
enum class EventPriority {
    Low = 0,      ///< 低优先级
    Normal = 1,   ///< 正常优先级（默认）
    High = 2,     ///< 高优先级
    Critical = 3  ///< 关键优先级
};

}

#endif
