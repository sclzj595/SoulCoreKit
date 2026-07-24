#ifndef SOUL_CORE_FACTORY_H
#define SOUL_CORE_FACTORY_H

#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <mutex>

namespace sc {

template<typename T>
class IFactory {
public:
    virtual ~IFactory() = default;

    virtual std::unique_ptr<T> create(const std::string& name) = 0;
};

template<typename T>
class Factory : public IFactory<T> {
public:
    using Creator = std::function<std::unique_ptr<T>()>;

    void registerCreator(const std::string& name, Creator creator) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_creators[name] = std::move(creator);
    }

    std::unique_ptr<T> create(const std::string& name) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_creators.find(name);
        if (it != m_creators.end()) {
            return it->second();
        }
        return nullptr;
    }

    bool hasCreator(const std::string& name) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_creators.find(name) != m_creators.end();
    }

    void unregisterCreator(const std::string& name) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_creators.erase(name);
    }

    size_t creatorCount() const {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_creators.size();
    }

private:
    std::unordered_map<std::string, Creator> m_creators;
    mutable std::mutex m_mutex;
};

}

#endif