#ifndef SOUL_CORE_APPLICATION_H
#define SOUL_CORE_APPLICATION_H

#include <QCoreApplication>
#include <functional>
#include <mutex>
#include <memory>
#include <vector>

namespace sc {

class Application {
public:
    using StartupCallback = std::function<bool()>;
    using ShutdownCallback = std::function<void()>;
    using ExceptionHandler = std::function<void(const std::exception&)>;
    using UnhandledExceptionHandler = std::function<void()>;

    Application(int& argc, char** argv);
    ~Application();

    int run();

    void setExitCode(int code);
    int exitCode() const;

    void addStartupCallback(StartupCallback callback);
    void addShutdownCallback(ShutdownCallback callback);

    void setExceptionHandler(ExceptionHandler handler);
    void setUnhandledExceptionHandler(UnhandledExceptionHandler handler);

    static Application* instance();
    static bool hasInstance();

    QCoreApplication* qApplication();

private:
    static Application* s_instance;
    static std::mutex s_mutex;

    std::unique_ptr<QCoreApplication> m_qApp;
    int m_exitCode = 0;

    std::vector<StartupCallback> m_startupCallbacks;
    std::vector<ShutdownCallback> m_shutdownCallbacks;

    ExceptionHandler m_exceptionHandler;
    UnhandledExceptionHandler m_unhandledExceptionHandler;

    bool executeStartupCallbacks();
    void executeShutdownCallbacks();
};

}

#endif