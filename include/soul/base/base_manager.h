#ifndef SOUL_BASE_BASE_MANAGER_H
#define SOUL_BASE_BASE_MANAGER_H

#include "soul/base/base_object.h"

namespace sc {

/**
 * @class BaseManager
 * @brief 管理器基类，提供初始化和清理生命周期
 *
 * BaseManager 是所有管理器类的基类，提供统一的初始化和清理接口。
 * 管理器通常是全局访问的单例或全局对象，负责管理一类资源或服务。
 *
 * 使用方式：
 * @code
 * class MyManager : public BaseManager {
 * public:
 *     bool init() override {
 *         // 初始化逻辑
 *         return true;
 *     }
 *     void shutdown() override {
 *         // 清理逻辑
 *     }
 * };
 *
 * MyManager manager;
 * manager.init();
 * // 使用...
 * manager.shutdown();
 * @endcode
 *
 * @see BaseObject, BaseService
 */
class BaseManager : public BaseObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit BaseManager(QObject* parent = nullptr);

    /**
     * @brief 初始化管理器
     *
     * 子类应重写此方法实现初始化逻辑。
     * 默认实现将 m_initialized 设置为 true。
     *
     * @return 初始化成功返回 true，失败返回 false
     */
    virtual bool init();

    /**
     * @brief 清理管理器
     *
     * 子类应重写此方法实现清理逻辑。
     * 默认实现将 m_initialized 设置为 false。
     */
    virtual void shutdown();

    /**
     * @brief 检查管理器是否已初始化
     * @return 如果已初始化返回 true，否则返回 false
     */
    bool isInitialized() const;

protected:
    bool m_initialized = false;
};

}

#endif
