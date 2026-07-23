#ifndef SOUL_STORAGE_I_SERIALIZER_H
#define SOUL_STORAGE_I_SERIALIZER_H

#include <QString>
#include <QByteArray>
#include <QVariant>
#include <memory>
#include "soul/core/result.h"

namespace sc {

/**
 * @class ISerializer
 * @brief 序列化器接口定义
 *
 * ISerializer 定义了统一的序列化/反序列化接口，支持将数据转换为字节数组
 * 以及将字节数组转换回原始数据。具体实现包括：
 * - JsonSerializer：JSON 格式序列化
 *
 * 使用方式：
 * @code
 * std::shared_ptr<ISerializer> serializer = std::make_shared<JsonSerializer>();
 * QVariant data = ...;
 * auto result = serializer->serialize(data);
 * if (result.isOk()) {
 *     QByteArray bytes = result.unwrap();
 * }
 * @endcode
 *
 * @see JsonSerializer
 */
class ISerializer {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~ISerializer() = default;

    /**
     * @brief 获取序列化器名称
     * @return 序列化器名称
     */
    virtual QString name() const = 0;

    /**
     * @brief 将数据序列化为字节数组
     * @param data 待序列化的数据
     * @return 包含字节数组的 Result
     */
    virtual Result<QByteArray> serialize(const QVariant& data) const = 0;

    /**
     * @brief 将字节数组反序列化为数据
     * @param data 待反序列化的字节数组
     * @return 包含数据的 Result
     */
    virtual Result<QVariant> deserialize(const QByteArray& data) const = 0;

    /**
     * @brief 将对象序列化为字节数组（模板方法）
     * @tparam T 对象类型
     * @param obj 待序列化的对象
     * @return 包含字节数组的 Result
     */
    template<typename T>
    Result<QByteArray> serializeObject(const T& obj) const {
        return serialize(QVariant::fromValue(obj));
    }

    /**
     * @brief 将字节数组反序列化为对象（模板方法）
     * @tparam T 对象类型
     * @param data 待反序列化的字节数组
     * @return 包含对象的 Result
     */
    template<typename T>
    Result<T> deserializeObject(const QByteArray& data) const {
        auto result = deserialize(data);
        if (result.isErr()) {
            return result.unwrapErr();
        }
        return result.unwrap().value<T>();
    }
};

/**
 * @brief 序列化器智能指针类型
 */
using SerializerPtr = std::shared_ptr<ISerializer>;

}

#endif
