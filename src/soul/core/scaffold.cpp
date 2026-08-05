#include "soul/core/scaffold.h"
#include "soul/core/module_registry.h"
#include "soul/logging/log_macros.h"

namespace sc {

Scaffold::Scaffold(int& argc, char** argv)
    : m_app(std::make_unique<Application>(argc, argv)) {}

Scaffold::~Scaffold() {
    shutdown();
}

Scaffold& Scaffold::use(Module& module) {
    m_app->use(module);
    return *this;
}

Scaffold& Scaffold::use(Module* module) {
    m_app->use(module);
    return *this;
}

Scaffold& Scaffold::scan(ModuleRegistry& registry) {
    m_app->scan(registry);
    return *this;
}

int Scaffold::run() {
    if (m_state != State::Uninitialized) {
        if (m_state == State::Running) {
            SC_ERROR("Scaffold::run() called while already running");
        } else {
            SC_ERROR("Scaffold::run() called after shutdown");
        }
        return -1;
    }

    m_initialized = true;

    // 委托给 Application::execute() 完成完整生命周期
    // execute() 返回后应用已停止,状态设为 Shutdown
    int exitCode = m_app->execute();
    m_state = State::Shutdown;
    return exitCode;
}

void Scaffold::shutdown() {
    if (!m_initialized) {
        return;
    }
    if (m_state == State::Shutdown) {
        return;  // 已关闭，防止重复调用 [v2.0.0]
    }
    m_initialized = false;
    m_state = State::Shutdown;
    m_app->shutdown();
}

} // namespace sc
