#ifndef SOUL_CORE_INTERFACE_H
#define SOUL_CORE_INTERFACE_H

#include <string>

namespace sc {

/**
 * @class IInterface
 * @brief 纯虚接口基类，便于RTTI与多态
 *
 * 所有接口类应继承自 IInterface，提供统一的接口名查询能力。
 * 便于在运行时识别对象类型和进行接口转换。
 */
class IInterface {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IInterface() = default;

    /**
     * @brief 获取接口名称
     * @return 接口的唯一标识名称
     */
    virtual std::string interfaceName() const = 0;
};

}

#endif
