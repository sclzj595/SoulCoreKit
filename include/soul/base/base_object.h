#ifndef SOUL_BASE_BASE_OBJECT_H
#define SOUL_BASE_BASE_OBJECT_H

#include <QObject>
#include <QString>

namespace sc {

/**
 * @class BaseObject
 * @brief 所有业务对象的基类，继承自QObject
 *
 * BaseObject 提供了统一的调试名称管理和对象路径获取功能。
 * 所有业务层的QObject派生类都应继承自此类。
 *
 * @see BaseManager, BaseService
 */
class BaseObject : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit BaseObject(QObject* parent = nullptr);

    /**
     * @brief 构造函数（带调试名称）
     * @param debugName 调试名称
     * @param parent 父对象
     */
    explicit BaseObject(const QString& debugName, QObject* parent = nullptr);

    /**
     * @brief 获取调试名称
     * @return 调试名称
     */
    QString debugName() const;

    /**
     * @brief 设置调试名称
     * @param name 调试名称
     */
    void setDebugName(const QString& name);

    /**
     * @brief 获取对象在对象树中的路径
     * @return 对象路径字符串
     */
    QString objectPath() const;

protected:
    QString m_debugName;
};

}

#endif
