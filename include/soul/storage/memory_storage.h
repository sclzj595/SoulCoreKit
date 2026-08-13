#ifndef SOUL_STORAGE_MEMORY_STORAGE_H
#define SOUL_STORAGE_MEMORY_STORAGE_H

#include "istorage.h"
#include <QHash>
#include <mutex>

namespace sc {

class MemoryStorage : public IStorage {
public:
    MemoryStorage();
    ~MemoryStorage() override = default;

    Result<void> open(const QString& path) override;
    void close() override;
    bool isOpen() const override;

    Result<void> put(const QString& key, const QString& value) override;
    Result<QString> get(const QString& key) const override;
    Result<void> remove(const QString& key) override;
    bool contains(const QString& key) const override;

    Result<void> putBytes(const QString& key, const QByteArray& value) override;
    Result<QByteArray> getBytes(const QString& key) const override;

    std::vector<QString> keys() const override;
    int count() const override;
    Result<void> clear() override;

private:
    QHash<QString, QString> m_stringData;
    QHash<QString, QByteArray> m_bytesData;
    bool m_isOpen;
    // [审计] MemoryStorage 是独立存储组件, 原实现全部方法无锁, 多线程读写
    // m_stringData/m_bytesData/m_isOpen 构成数据竞争。加内部锁使其线程安全。
    mutable std::mutex m_mutex;
};

}

#endif
