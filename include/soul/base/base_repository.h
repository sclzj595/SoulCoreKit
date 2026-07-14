#ifndef SOUL_BASE_BASE_REPOSITORY_H
#define SOUL_BASE_BASE_REPOSITORY_H

#include "soul/core/result.h"
#include <string>
#include <vector>

namespace sc {

/**
 * @class IRepository
 * @brief 仓库接口，定义数据访问层的标准操作
 *
 * IRepository 是泛型接口，定义了数据访问层的基本CRUD操作。
 * 业务层的Repository应实现此接口。
 *
 * @tparam T 实体类型
 * @see BaseRepository, Result
 */
template<typename T>
class IRepository {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IRepository() = default;

    /**
     * @brief 根据ID查找实体
     * @param id 实体ID
     * @return 包含实体的Result，如果未找到则返回Err
     */
    virtual Result<T> findById(const std::string& id) = 0;

    /**
     * @brief 查找所有实体
     * @return 包含实体列表的Result
     */
    virtual Result<std::vector<T>> findAll() = 0;

    /**
     * @brief 保存实体
     * @param entity 实体对象
     * @return 保存成功返回 true，失败返回 false
     */
    virtual bool save(const T& entity) = 0;

    /**
     * @brief 删除实体
     * @param id 实体ID
     * @return 删除成功返回 true，失败返回 false
     */
    virtual bool remove(const std::string& id) = 0;
};

/**
 * @class BaseRepository
 * @brief Repository基类的默认实现（字符串实体）
 *
 * BaseRepository 是 IRepository<std::string> 的默认实现，
 * 提供了基础的空实现，供其他Repository继承使用。
 *
 * @see IRepository
 */
class BaseRepository : public IRepository<std::string> {
public:
    /**
     * @brief 根据ID查找实体
     * @param id 实体ID
     * @return 包含实体的Result
     */
    Result<std::string> findById(const std::string& id) override;

    /**
     * @brief 查找所有实体
     * @return 包含实体列表的Result
     */
    Result<std::vector<std::string>> findAll() override;

    /**
     * @brief 保存实体
     * @param entity 实体对象
     * @return 保存成功返回 true，失败返回 false
     */
    bool save(const std::string& entity) override;

    /**
     * @brief 删除实体
     * @param id 实体ID
     * @return 删除成功返回 true，失败返回 false
     */
    bool remove(const std::string& id) override;
};

}

#endif
