#ifndef SOUL_STORAGE_FILE_STORAGE_H
#define SOUL_STORAGE_FILE_STORAGE_H

#include "istorage.h"

namespace sc {

/**
 * @class FileStorage
 * @brief 文件系统存储实现
 *
 * FileStorage 将数据存储在文件系统中，支持持久化存储。
 * 每个键对应一个文件，支持字符串和二进制数据。
 *
 * 使用方式：
 * @code
 * FileStorage storage;
 * storage.open("/data/storage");
 * storage.put("config.json", "{\"key\": \"value\"}");
 * QString config = storage.get("config.json");
 * @endcode
 *
 * @see IStorage, MemoryStorage
 */
class FileStorage : public IStorage {
public:
    /**
     * @brief 构造函数
     */
    FileStorage();

    /**
     * @brief 析构函数
     */
    ~FileStorage() override = default;

    /**
     * @brief 打开文件存储
     * @param path 基础目录路径
     * @return Result<void>，成功返回 Ok
     */
    Result<void> open(const QString& path) override;

    /**
     * @brief 关闭文件存储
     */
    void close() override;

    /**
     * @brief 检查是否已打开
     * @return 如果已打开返回 true
     */
    bool isOpen() const override;

    /**
     * @brief 存储字符串值
     * @param key 键（文件名）
     * @param value 值
     * @return Result<void>，成功返回 Ok
     */
    Result<void> put(const QString& key, const QString& value) override;

    /**
     * @brief 获取字符串值
     * @param key 键（文件名）
     * @return Result<QString>，成功返回 Ok(value)，失败返回 Error
     */
    Result<QString> get(const QString& key) const override;

    /**
     * @brief 删除指定键（文件）
     * @param key 键（文件名）
     * @return Result<void>，成功返回 Ok
     */
    Result<void> remove(const QString& key) override;

    /**
     * @brief 检查键（文件）是否存在
     * @param key 键（文件名）
     * @return 如果存在返回 true
     */
    bool contains(const QString& key) const override;

    /**
     * @brief 存储二进制数据
     * @param key 键（文件名）
     * @param value 二进制数据
     * @return Result<void>，成功返回 Ok
     */
    Result<void> putBytes(const QString& key, const QByteArray& value) override;

    /**
     * @brief 获取二进制数据
     * @param key 键（文件名）
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
     * @brief 清空所有数据（删除所有文件）
     * @return Result<void>，成功返回 Ok
     */
    Result<void> clear() override;

private:
    /**
     * @brief 获取键对应的文件路径
     * @param key 键
     * @return 文件路径
     */
    QString getFilePath(const QString& key) const;

    QString m_basePath;
    bool m_isOpen;
};

}

#endif
