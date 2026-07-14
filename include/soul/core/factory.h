#ifndef SOUL_CORE_FACTORY_H
#define SOUL_CORE_FACTORY_H

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>

namespace sc {

/**
 * @class IFactory
 * @brief 抽象工厂接口
 *
 * 定义工厂模式的通用接口，支持按名称创建对象。
 *
 * @tparam T 产品类型
 */
template<typename T>
class IFactory {
public:
    /**
     * @brief 虚析构函数
     */
    virtual ~IFactory() = default;

    /**
     * @brief 根据名称创建对象
     * @param name 产品名称
     * @return 创建的对象指针，如果找不到则返回 nullptr
     */
    virtual std::unique_ptr<T> create(const std::string& name) = 0;
};

/**
 * @class Factory
 * @brief 工厂模式的默认实现
 *
 * Factory 是一个通用的对象创建工厂，支持注册创建函数和按名称创建对象。
 * 使用函数指针存储创建逻辑，支持运行时动态扩展。
 *
 * @tparam T 产品类型
 * @see IFactory
 */
template<typename T>
class Factory : public IFactory<T> {
public:
    /**
     * @brief 创建函数类型
     */
    using Creator = std::function<std::unique_ptr<T>()>;

    /**
     * @brief 注册创建函数
     * @param name 产品名称
     * @param creator 创建函数，返回对象的 unique_ptr
     */
    void registerCreator(const std::string& name, Creator creator) {
        m_creators[name] = std::move(creator);
    }

    /**
     * @brief 根据名称创建对象
     * @param name 产品名称
     * @return 创建的对象指针，如果找不到则返回 nullptr
     */
    std::unique_ptr<T> create(const std::string& name) override {
        auto it = m_creators.find(name);
        if (it != m_creators.end()) {
            return it->second();
        }
        return nullptr;
    }

private:
    std::unordered_map<std::string, Creator> m_creators;
};

}

#endif
