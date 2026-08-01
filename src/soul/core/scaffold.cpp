#include "soul/core/scaffold.h"
#include "soul/core/module_registry.h"
#include "soul/logging/log_macros.h"
#include <algorithm>
#include <QString>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace sc {

Scaffold::Scaffold(int& argc, char** argv)
    : m_app(std::make_unique<Application>(argc, argv)) {}

Scaffold::~Scaffold() {
    shutdown();
}

Scaffold& Scaffold::use(Module& module) {
    m_modules.push_back(&module);
    return *this;
}

Scaffold& Scaffold::use(Module* module) {
    if (module) {
        m_modules.push_back(module);
    }
    return *this;
}

Scaffold& Scaffold::scan(ModuleRegistry& registry) {
    auto factories = registry.factories();
    for (auto& [name, factory] : factories) {
        auto module = factory();
        if (module) {
            SC_INFO(std::string("Scaffold: auto-discovered module '") + name + "'");
            m_modules.push_back(module.get());
            m_ownedModules.push_back(std::move(module));
        }
    }
    return *this;
}

namespace {
// 拓扑排序 + 优先级排序:依赖关系优先,同层按 priority 降序,再按注册顺序。
// 返回排序后的模块指针列表;检测到循环依赖时返回空。
std::vector<Module*> topoSortWithPriority(const std::vector<Module*>& modules) {
    // 名称 -> 模块索引
    std::unordered_map<std::string, std::size_t> nameToIdx;
    for (std::size_t i = 0; i < modules.size(); ++i) {
        nameToIdx[modules[i]->name()] = i;
    }

    // 构建邻接表 + 入度(被依赖者先 init,所以边: dep -> module)
    std::vector<std::vector<std::size_t>> graph(modules.size());
    std::vector<std::size_t> indegree(modules.size(), 0);
    for (std::size_t i = 0; i < modules.size(); ++i) {
        for (const auto& dep : modules[i]->dependsOn()) {
            auto it = nameToIdx.find(dep);
            if (it == nameToIdx.end()) {
                SC_ERROR(std::string("Scaffold: module '") + modules[i]->name() +
                         "' depends on unknown module '" + dep + "'");
                return {};
            }
            graph[it->second].push_back(i);  // dep -> module
            ++indegree[i];
        }
    }

    // 拓扑排序:每次从入度为 0 的节点中选 priority 最大(再按注册顺序)的
    std::vector<Module*> result;
    result.reserve(modules.size());
    std::vector<bool> visited(modules.size(), false);

    while (result.size() < modules.size()) {
        // 候选:入度为 0 且未访问
        int bestIdx = -1;
        int bestPriority = 0;
        for (std::size_t i = 0; i < modules.size(); ++i) {
            if (!visited[i] && indegree[i] == 0) {
                int p = modules[i]->priority();
                if (bestIdx == -1 || p > bestPriority) {
                    bestIdx = static_cast<int>(i);
                    bestPriority = p;
                }
            }
        }
        if (bestIdx == -1) {
            // 剩余节点都有入度 -> 循环依赖
            SC_ERROR("Scaffold: circular dependency detected among modules");
            return {};
        }
        visited[bestIdx] = true;
        result.push_back(modules[bestIdx]);
        for (std::size_t next : graph[bestIdx]) {
            if (indegree[next] > 0) --indegree[next];
        }
    }
    return result;
}
} // namespace

int Scaffold::run() {
    // [v1.9.2] 状态检查: 仅允许从 Uninitialized 状态调用
    if (m_state != State::Uninitialized) {
        if (m_state == State::Running) {
            SC_ERROR("Scaffold::run() called while already running");
        } else {
            SC_ERROR("Scaffold::run() called after shutdown");
        }
        return -1;
    }

    // 1. 过滤 isEnabled() == false 的模块
    std::vector<Module*> enabledModules;
    for (auto* m : m_modules) {
        if (m->isEnabled()) {
            enabledModules.push_back(m);
        } else {
            SC_INFO(std::string("Scaffold: module '") + m->name() + "' disabled, skipped");
        }
    }

    // 2. 拓扑排序 + 优先级排序
    auto sorted = topoSortWithPriority(enabledModules);
    if (sorted.empty() && !enabledModules.empty()) {
        SC_ERROR("Scaffold: module sorting failed (circular dependency or missing dep)");
        return -1;
    }

    // 保存拓扑排序结果,供 shutdown() 按逆序 cleanup(仅含 enabled 模块)
    m_sortedModules = sorted;

    // 3. 按排序顺序执行 init()
    std::vector<Module*> initializedModules;
    for (auto* module : sorted) {
        const std::string name = module->name();
        auto result = module->init();
        if (!result.isOk()) {
            const QString msg = result.unwrapErr().message();
            SC_ERROR(QString("Scaffold: module '%1' init failed: %2")
                         .arg(QString::fromStdString(name))
                         .arg(msg)
                         .toStdString());
            // 按逆序回滚已成功 init 的模块
            for (auto it = initializedModules.rbegin(); it != initializedModules.rend(); ++it) {
                const std::string rname = (*it)->name();
                try {
                    (*it)->cleanup();
                    SC_INFO(std::string("Scaffold: rolled back module '") + rname + "'");
                } catch (const std::exception& e) {
                    SC_ERROR(std::string("Scaffold: rollback of module '") + rname +
                             "' failed: " + e.what());
                }
            }
            return -1;
        }
        initializedModules.push_back(module);
        SC_INFO(std::string("Scaffold: module '") + name + "' initialized");
    }

    // 4. 按排序顺序执行 onStart()
    std::vector<Module*> startedModules;
    for (auto* module : initializedModules) {
        const std::string name = module->name();
        auto result = module->onStart();
        if (!result.isOk()) {
            const QString msg = result.unwrapErr().message();
            SC_ERROR(QString("Scaffold: module '%1' start failed: %2")
                         .arg(QString::fromStdString(name))
                         .arg(msg)
                         .toStdString());
            // 逆序 stop 已 start 的模块
            for (auto it = startedModules.rbegin(); it != startedModules.rend(); ++it) {
                try { (*it)->onStop(); } catch (const std::exception& e) {
                    SC_ERROR(std::string("Scaffold: stop of '") + (*it)->name() +
                             "' failed: " + e.what());
                }
            }
            // 逆序 cleanup 已 init 的模块
            for (auto it = initializedModules.rbegin(); it != initializedModules.rend(); ++it) {
                try { (*it)->cleanup(); } catch (const std::exception& e) {
                    SC_ERROR(std::string("Scaffold: cleanup of '") + (*it)->name() +
                             "' failed: " + e.what());
                }
            }
            return -1;
        }
        startedModules.push_back(module);
        SC_INFO(std::string("Scaffold: module '") + name + "' started");
    }

    m_initialized = true;
    m_state = State::Running;  // [v1.9.2] 状态跟踪

    // 5. 进入事件循环
    // 注意: 不再注册 shutdown 回调到 Application。~Scaffold() 函数体会先于成员析构
    // 调用 shutdown() 完成全部 cleanup,此时 m_initialized 置 false。随后 m_app 析构
    // 触发 ~Application(),若回调再次调用 shutdown() 会读取已析构的 m_initialized(UB)。
    // 析构函数已保证 cleanup 执行,回调冗余且有害,故移除。
    return m_app->run();
}

void Scaffold::shutdown() {
    if (!m_initialized) {
        return;
    }
    // 按拓扑逆序执行 onStop() + cleanup()(依赖者先 cleanup,被依赖者后 cleanup)
    for (auto it = m_sortedModules.rbegin(); it != m_sortedModules.rend(); ++it) {
        const std::string name = (*it)->name();
        try {
            (*it)->onStop();
            SC_INFO(std::string("Scaffold: module '") + name + "' stopped");
        } catch (const std::exception& e) {
            SC_ERROR(std::string("Scaffold: module '") + name +
                     "' onStop failed: " + e.what());
        }
        try {
            (*it)->cleanup();
            SC_INFO(std::string("Scaffold: module '") + name + "' cleaned up");
        } catch (const std::exception& e) {
            SC_ERROR(std::string("Scaffold: module '") + name +
                     "' cleanup failed: " + e.what());
        }
    }
    m_initialized = false;
    m_state = State::Shutdown;  // [v1.9.2] 状态跟踪
}

} // namespace sc
