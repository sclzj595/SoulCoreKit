#ifndef SOUL_STORAGE_ISTORAGE_H
#define SOUL_STORAGE_ISTORAGE_H

#include <QString>
#include <QByteArray>
#include <vector>
#include "soul/core/result.h"
#include "soul/core/interface.h"

namespace sc {

/**
 * @class IStorage
 * @brief 存储接口定义
 *
 * IStorage 定义了统一的存储抽象接口，支持字符串和二进制数据的存储。
 * 具体实现包括：
 * - MemoryStorage：内存存储（进程生命周期内有效）
 * - FileStorage：文件系统存储（持久化）
 *
 * 使用方式：
 * @code
 * std::shared_ptr<IStorage> storage = std::make_shared<MemoryStorage>();
 * auto result = storage->open("memory");
 * if (result.isOk()) {
 *     storage->put("key", "value");
 *     QString value = storage->get("key");
 * }
 * @endcode
 *
 * @see MemoryStorage, FileStorage, IInterface
 */
class IStorage : public IInterface {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IStorage() = default;

    /**
     * @brief 获取接口名称
     * @return 接口名称 "IStorage"
     */
    std::string interfaceName() const override { return "IStorage"; }

    /**
     * @brief 打开存储
     * @param path 存储路径（不同实现含义不同）
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> open(const QString& path) = 0;

    /**
     * @brief 关闭存储
     */
    virtual void close() = 0;

    /**
     * @brief 检查存储是否已打开
     * @return 如果已打开返回 true
     */
    virtual bool isOpen() const = 0;

    /**
     * @brief 存储字符串值
     * @param key 键
     * @param value 值
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> put(const QString& key, const QString& value) = 0;

    /**
     * @brief 获取字符串值
     * @param key 键
     * @return Result<QString>，成功返回 Ok(value)，失败返回 Error
     */
    virtual Result<QString> get(const QString& key) const = 0;

    /**
     * @brief 删除指定键
     * @param key 键
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> remove(const QString& key) = 0;

    /**
     * @brief 检查键是否存在
     * @param key 键
     * @return 如果存在返回 true
     */
    virtual bool contains(const QString& key) const = 0;

    /**
     * @brief 存储二进制数据
     * @param key 键
     * @param value 二进制数据
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> putBytes(const QString& key, const QByteArray& value) = 0;

    /**
     * @brief 获取二进制数据
     * @param key 键
     * @return Result<QByteArray>，成功返回 Ok(data)，失败返回 Error
     */
    virtual Result<QByteArray> getBytes(const QString& key) const = 0;

    /**
     * @brief 获取所有键
     * @return 键的列表
     */
    virtual std::vector<QString> keys() const = 0;

    /**
     * @brief 获取键值对数量
     * @return 数量
     */
    virtual int count() const = 0;

    /**
     * @brief 清空所有数据
     * @return Result<void>，成功返回 Ok，失败返回 Error
     */
    virtual Result<void> clear() = 0;
};

}

#endif
