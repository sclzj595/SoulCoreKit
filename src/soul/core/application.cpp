#include "soul/core/application.h"
#include "soul/logging/log_macros.h"
#include <mutex>
#include <string>

namespace sc {

Application* Application::s_instance = nullptr;
std::mutex Application::s_mutex;

Application::Application(int& argc, char** argv)
    : m_qApp(std::make_unique<QCoreApplication>(argc, argv)) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_instance = this;
}

Application::~Application() {
    executeShutdownCallbacks();
    std::lock_guard<std::mutex> lock(s_mutex);
    s_instance = nullptr;
}

int Application::run() {
    try {
        if (!executeStartupCallbacks()) {
            SC_ERROR("Application startup failed");
            return -1;
        }

        SC_INFO("Application started");
        m_exitCode = m_qApp->exec();
        SC_INFO("Application exiting with code: " + QString::number(m_exitCode).toStdString());

        return m_exitCode;

    } catch (const std::exception& e) {
        if (m_exceptionHandler) {
            m_exceptionHandler(e);
        } else {
            SC_ERROR("Unhandled exception: " + std::string(e.what()));
        }
        return -1;

    } catch (...) {
        if (m_unhandledExceptionHandler) {
            m_unhandledExceptionHandler();
        } else {
            SC_ERROR("Unhandled unknown exception");
        }
        return -1;
    }
}

void Application::setExitCode(int code) {
    m_exitCode = code;
}

int Application::exitCode() const {
    return m_exitCode;
}

void Application::addStartupCallback(StartupCallback callback) {
    m_startupCallbacks.push_back(std::move(callback));
}

void Application::addShutdownCallback(ShutdownCallback callback) {
    m_shutdownCallbacks.push_back(std::move(callback));
}

void Application::setExceptionHandler(ExceptionHandler handler) {
    m_exceptionHandler = std::move(handler);
}

void Application::setUnhandledExceptionHandler(UnhandledExceptionHandler handler) {
    m_unhandledExceptionHandler = std::move(handler);
}

Application* Application::instance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_instance;
}

bool Application::hasInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_instance != nullptr;
}

QCoreApplication* Application::qApplication() {
    return m_qApp.get();
}

bool Application::executeStartupCallbacks() {
    for (const auto& callback : m_startupCallbacks) {
        if (!callback()) {
            return false;
        }
    }
    return true;
}

void Application::executeShutdownCallbacks() {
    for (const auto& callback : m_shutdownCallbacks) {
        try {
            callback();
        } catch (const std::exception& e) {
            SC_ERROR("Error in shutdown callback: " + std::string(e.what()));
        }
    }
}

}