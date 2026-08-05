#ifndef SOUL_CS_WINDOW_MANAGER_H
#define SOUL_CS_WINDOW_MANAGER_H

// ============================================================================
// cs_window_manager.h — CS 窗口生命周期管理器 [v2.5.0]
// ============================================================================
//
// 对标 Spring 的 Session/Window 管理。
// 包装 sc::Window，提供统一窗口生命周期管理。
//
// 关系: 包装 sc::Window，管理多窗口的打开/聚焦/关闭/列表。
// ============================================================================

#include <QObject>
#include <QHash>
#include <QString>
#include <QVariantMap>
#include <memory>

#include "soul/cs/cs_global.h"

namespace sc {
class Window;
}

namespace sc::cs {

/// @brief CS 窗口管理器（对标 Spring 的 Session/Window 管理）
///
/// 单例模式，管理所有窗口的生命周期。
/// 支持按名称打开/聚焦/关闭窗口。
///
/// @par 使用示例
/// @code
/// auto& wm = CsWindowManager::instance();
/// wm.open("player", {{"songId", 123}});
/// wm.focus("player");
/// wm.close("player");
/// QList<QString> windows = wm.list();
/// @endcode
class CsWindowManager : public QObject {
    Q_OBJECT

public:
    /// @brief 获取单例
    static CsWindowManager& instance();

    /// @brief 打开新窗口
    /// @param windowName 窗口名称
    /// @param params 窗口参数
    /// @return 窗口指针，失败返回 nullptr
    sc::Window* open(const QString& windowName, const QVariantMap& params = {});

    /// @brief 聚焦已有窗口
    /// @param windowName 窗口名称
    /// @return true 成功聚焦
    bool focus(const QString& windowName);

    /// @brief 关闭窗口
    /// @param windowName 窗口名称
    /// @return true 成功关闭
    bool close(const QString& windowName);

    /// @brief 关闭所有窗口
    void closeAll();

    /// @brief 列出所有已打开窗口
    /// @return 窗口名称列表
    QList<QString> list() const;

    /// @brief 检查窗口是否已打开
    /// @param windowName 窗口名称
    bool isOpen(const QString& windowName) const;

    /// @brief 获取窗口指针
    /// @param windowName 窗口名称
    /// @return 窗口指针，不存在返回 nullptr
    sc::Window* window(const QString& windowName) const;

    /// @brief 注册窗口工厂函数
    /// @param windowName 窗口名称
    /// @param factory 工厂函数
    void registerFactory(const QString& windowName,
                         std::function<sc::Window*(const QVariantMap&)> factory);

signals:
    /// @brief 窗口已打开信号
    void windowOpened(const QString& windowName);

    /// @brief 窗口已关闭信号
    void windowClosed(const QString& windowName);

    /// @brief 窗口已聚焦信号
    void windowFocused(const QString& windowName);

private:
    CsWindowManager();
    ~CsWindowManager() override;
    CsWindowManager(const CsWindowManager&) = delete;
    CsWindowManager& operator=(const CsWindowManager&) = delete;

    struct WindowEntry {
        sc::Window* window = nullptr;
        std::function<sc::Window*(const QVariantMap&)> factory;
    };

    QHash<QString, WindowEntry> m_windows;
};

} // namespace sc::cs

#endif // SOUL_CS_WINDOW_MANAGER_H