// ============================================================================
// cs_window_manager.cpp — CS 窗口生命周期管理器实现 [v2.1.0]
// ============================================================================

#include "soul/cs/cs_window_manager.h"
#include "soul/ui/window.h"
#include <QDebug>

namespace sc::cs {

CsWindowManager::CsWindowManager()
    : QObject(nullptr)
{
}

CsWindowManager::~CsWindowManager() {
    // 对标 Spring 的 Session 销毁 — 安全清理窗口引用
    // 不调用 closeAll()，因为析构阶段 Qt 对象树可能已部分销毁，
    // 信号连接可能处于未定义状态。QWidget 窗口由 Qt parent-child 机制自动销毁。
    m_windows.clear();
}

CsWindowManager& CsWindowManager::instance() {
    static CsWindowManager manager;
    return manager;
}

sc::Window* CsWindowManager::open(const QString& windowName, const QVariantMap& params) {
    // 如果窗口已存在，聚焦后返回
    if (m_windows.contains(windowName)) {
        auto& entry = m_windows[windowName];
        if (entry.window) {
            entry.window->show();
            entry.window->raise();
            entry.window->activateWindow();
            emit windowFocused(windowName);
            return entry.window;
        }
    }

    // 使用工厂函数创建窗口
    WindowEntry entry;
    auto factoryIt = m_windows.find(windowName);
    if (factoryIt != m_windows.end() && factoryIt->factory) {
        entry.window = factoryIt->factory(params);
    }

    if (!entry.window) {
        // 无工厂函数，创建默认窗口
        entry.window = new sc::Window();
        entry.window->setTitle(windowName);
    }

    // 连接窗口关闭信号
    QObject::connect(entry.window, &sc::Window::windowClosed, this, [this, windowName]() {
        m_windows.remove(windowName);
        emit windowClosed(windowName);
    });

    entry.factory = (factoryIt != m_windows.end()) ? factoryIt->factory : nullptr;
    m_windows.insert(windowName, entry);

    entry.window->show();
    emit windowOpened(windowName);
    return entry.window;
}

bool CsWindowManager::focus(const QString& windowName) {
    auto it = m_windows.find(windowName);
    if (it != m_windows.end() && it->window) {
        it->window->show();
        it->window->raise();
        it->window->activateWindow();
        emit windowFocused(windowName);
        return true;
    }
    return false;
}

bool CsWindowManager::close(const QString& windowName) {
    auto it = m_windows.find(windowName);
    if (it != m_windows.end() && it->window) {
        it->window->close();
        // windowClosed 信号会触发清理
        return true;
    }
    return false;
}

void CsWindowManager::closeAll() {
    // 复制窗口名称列表，避免在迭代中修改
    QStringList names;
    for (auto it = m_windows.constBegin(); it != m_windows.constEnd(); ++it) {
        names.append(it.key());
    }
    for (const QString& name : names) {
        close(name);
    }
}

QList<QString> CsWindowManager::list() const {
    return m_windows.keys();
}

bool CsWindowManager::isOpen(const QString& windowName) const {
    return m_windows.contains(windowName) && m_windows[windowName].window != nullptr;
}

sc::Window* CsWindowManager::window(const QString& windowName) const {
    auto it = m_windows.find(windowName);
    if (it != m_windows.end()) {
        return it->window;
    }
    return nullptr;
}

void CsWindowManager::registerFactory(const QString& windowName,
                                       std::function<sc::Window*(const QVariantMap&)> factory) {
    // 保留已有窗口，仅更新工厂函数
    auto it = m_windows.find(windowName);
    if (it != m_windows.end()) {
        it->factory = std::move(factory);
    } else {
        WindowEntry entry;
        entry.factory = std::move(factory);
        m_windows.insert(windowName, entry);
    }
}

} // namespace sc::cs
