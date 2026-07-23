#ifndef SOUL_BASE_BASE_SERVICE_H
#define SOUL_BASE_BASE_SERVICE_H

#include "soul/base/base_object.h"

namespace sc {

/**
 * @class BaseService
 * @brief 服务基类，提供启动和停止生命周期
 *
 * BaseService 是所有服务类的基类，提供统一的启动和停止接口。
 * 服务通常是业务逻辑的容器，负责执行特定的业务功能。
 *
 * 使用方式：
 * @code
 * class MyService : public BaseService {
 * public:
 *     bool start() override {
 *         // 启动服务逻辑
 *         return true;
 *     }
 *     void stop() override {
 *         // 停止服务逻辑
 *     }
 * };
 *
 * MyService service;
 * service.start();
 * // 使用...
 * service.stop();
 * @endcode
 *
 * @see BaseObject, BaseManager
 */
class BaseService : public BaseObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit BaseService(QObject* parent = nullptr);

    /**
     * @brief 启动服务
     *
     * 子类应重写此方法实现服务启动逻辑。
     * 默认实现将 m_running 设置为 true。
     *
     * @return 启动成功返回 true，失败返回 false
     */
    virtual bool start();

    /**
     * @brief 停止服务
     *
     * 子类应重写此方法实现服务停止逻辑。
     * 默认实现将 m_running 设置为 false。
     */
    virtual void stop();

    /**
     * @brief 检查服务是否正在运行
     * @return 如果正在运行返回 true，否则返回 false
     */
    bool isRunning() const;

protected:
    bool m_running = false;
};

}

#endif
