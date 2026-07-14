#ifndef SOUL_STORAGE_MEMORY_STORAGE_H
#define SOUL_STORAGE_MEMORY_STORAGE_H

#include "istorage.h"
#include <QHash>

namespace sc {

/**
 * @class MemoryStorage
 * @brief 内存存储实现
 *
 * MemoryStorage 将数据存储在内存中，进程退出后数据丢失。
 * 使用 QHash 作为底层存储，支持字符串和二进制数据。
 *
 * 使用方式：
 * @code
 * MemoryStorage storage;
 * storage.open("memory");
 * storage.put("username", "admin");
 * QString username = storage.get("username");
 * @endcode
 *
 * @see IStorage, FileStorage
 */
class MemoryStorage : public IStorage {
public:
    /**
     * @brief 构造函数
     */
    MemoryStorage();

    /**
     * @brief 析构函数
     */
    ~MemoryStorage() override = default;

    /**
     * @brief 打开内存存储（初始化）
     * @param path 存储标识（未使用）
     * @return Result<void>，成功返回 Ok
     */
    Result<void> open(const QString& path) override;

    /**
     * @brief 关闭内存存储（清空数据）
     */
    void close() override;

    /**
     * @brief 检查是否已打开
     * @return 如果已打开返回 true
     */
    bool isOpen() const override;

    /**
     * @brief 存储字符串值
     * @param key 键
     * @param value 值
     * @return Result<void>，成功返回 Ok
     */
    Result<void> put(const QString& key, const QString& value) override;

    /**
     * @brief 获取字符串值
     * @param key 键
     * @return Result<QString>，成功返回 Ok(value)，失败返回 Error
     */
    Result<QString> get(const QString& key) const override;

    /**
     * @brief 删除指定键
     * @param key 键
     * @return Result<void>，成功返回 Ok
     */
    Result<void> remove(const QString& key) override;

    /**
     * @brief 检查键是否存在
     * @param key 键
     * @return 如果存在返回 true
     */
    bool contains(const QString& key) const override;

    /**
     * @brief 存储二进制数据
     * @param key 键
     * @param value 二进制数据
     * @return Result<void>，成功返回 Ok
     */
    Result<void> putBytes(const QString& key, const QByteArray& value) override;

    /**
     * @brief 获取二进制数据
     * @param key 键
     * @return Result<QByteArray>，成功返回 Ok(data)，失败返回 Error
     */
    Result<QByteArray> getBytes(const QString& key) const override;

    /**
     * @brief 获取所有键
     * @return 键的列表
     */
    std::vector<QString> keys() const override;

    /**
     * @brief 获取键值对数量
     * @return 数量
     */
    int count() const override;

    /**
     * @brief 清空所有数据
     * @return Result<void>，成功返回 Ok
     */
    Result<void> clear() override;

private:
    QHash<QString, QString> m_stringData;
    QHash<QString, QByteArray> m_bytesData;
    bool m_isOpen;
};

}

#endif
