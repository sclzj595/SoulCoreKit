#include "soul/core/module_registry.h"

namespace sc {

void ModuleRegistry::registerModule(const std::string& name, Factory factory) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_factories[name] = std::move(factory);
}

void ModuleRegistry::unregisterModule(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_factories.erase(name);
}

std::map<std::string, ModuleRegistry::Factory> ModuleRegistry::factories() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_factories;  // 返回快照
}

bool ModuleRegistry::empty() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_factories.empty();
}

size_t ModuleRegistry::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_factories.size();
}

void ModuleRegistry::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_factories.clear();
}

} // namespace sc