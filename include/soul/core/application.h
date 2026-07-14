#ifndef SOUL_CORE_APPLICATION_H
#define SOUL_CORE_APPLICATION_H

#include <QCoreApplication>
#include <functional>
#include <memory>
#include <vector>

namespace sc {

/**
 * @class Application
 * @brief 应用程序封装类，提供生命周期管理和异常处理
 *
 * Application 封装了 QCoreApplication，提供统一的应用生命周期管理：
 * - 启动回调链（StartupCallback）
 * - 关闭回调链（ShutdownCallback）
 * - 异常处理机制
 * - 退出码管理
 *
 * @code
 * int main(int argc, char* argv[]) {
 *     sc::Application app(argc, argv);
 *     app.addStartupCallback([]() {
 *         // 初始化逻辑
 *         return true;
 *     });
 *     return app.run();
 * }
 * @endcode
 */
class Application {
public:
    /**
     * @brief 启动回调类型
     * @return 返回 false 可中止启动流程
     */
    using StartupCallback = std::function<bool()>;

    /**
     * @brief 关闭回调类型
     */
    using ShutdownCallback = std::function<void()>;

    /**
     * @brief 异常处理回调类型
     */
    using ExceptionHandler = std::function<void(const std::exception&)>;

    /**
     * @brief 未处理异常回调类型
     */
    using UnhandledExceptionHandler = std::function<void()>;

    /**
     * @brief 构造函数
     * @param argc 命令行参数数量
     * @param argv 命令行参数数组
     */
    Application(int& argc, char** argv);

    /**
     * @brief 析构函数
     */
    ~Application();

    /**
     * @brief 运行应用程序
     * @return 退出码
     */
    int run();

    /**
     * @brief 设置退出码
     * @param code 退出码
     */
    void setExitCode(int code);

    /**
     * @brief 获取退出码
     * @return 当前退出码
     */
    int exitCode() const;

    /**
     * @brief 添加启动回调
     * @param callback 启动回调函数
     */
    void addStartupCallback(StartupCallback callback);

    /**
     * @brief 添加关闭回调
     * @param callback 关闭回调函数
     */
    void addShutdownCallback(ShutdownCallback callback);

    /**
     * @brief 设置异常处理器
     * @param handler 异常处理函数
     */
    void setExceptionHandler(ExceptionHandler handler);

    /**
     * @brief 设置未处理异常处理器
     * @param handler 未处理异常处理函数
     */
    void setUnhandledExceptionHandler(UnhandledExceptionHandler handler);

    /**
     * @brief 获取应用实例
     * @return Application 实例指针
     */
    static Application* instance();

    /**
     * @brief 获取底层 QCoreApplication
     * @return QCoreApplication 指针
     */
    QCoreApplication* qApplication();

private:
    static Application* s_instance;

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
