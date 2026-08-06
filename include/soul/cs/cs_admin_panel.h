#ifndef SOUL_CS_ADMIN_PANEL_H
#define SOUL_CS_ADMIN_PANEL_H

// ============================================================================
// cs_admin_panel.h — 管理后台面板 [v2.5.0]
// ============================================================================
// 对标 Spring Boot Admin，提供 CS 应用的管理后台面板。
// 包括: 服务信息、健康检查、指标监控、日志管理、配置管理、线程转储。
// ============================================================================

#include <QWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <functional>

#include "soul/cs/cs_global.h"

// 前向声明: 以下头文件仅在 .cpp 实现中需要
//   - soul/observability/metrics.h  (Metrics 数据提供者)
//   - soul/server/health.h          (Health 数据提供者)

namespace sc::cs {

// ============================================================================
// CsAdminPanel — 管理后台面板
// ============================================================================
class CsAdminPanel : public QWidget {
    Q_OBJECT
public:
    explicit CsAdminPanel(QWidget* parent = nullptr);
    ~CsAdminPanel() override;

    // === 数据源配置 ===
    void setHealthEndpoint(std::function<QJsonObject()> healthProvider);
    void setMetricsEndpoint(std::function<QJsonObject()> metricsProvider);
    void setInfoEndpoint(std::function<QJsonObject()> infoProvider);
    void setEnvEndpoint(std::function<QJsonObject()> envProvider);
    void setThreadDumpEndpoint(std::function<QJsonObject()> threadDumpProvider);

    // === 刷新控制 ===
    void setAutoRefreshInterval(int ms);
    void startAutoRefresh();
    void stopAutoRefresh();
    void refreshAll();

    // === 面板访问 ===
    QTabWidget* tabWidget() const { return m_tabWidget; }

private slots:
    void onRefreshHealth();
    void onRefreshMetrics();
    void onRefreshInfo();
    void onRefreshEnv();
    void onRefreshThreadDump();

private:
    void setupUI();
    QWidget* createHealthPanel();
    QWidget* createMetricsPanel();
    QWidget* createInfoPanel();
    QWidget* createEnvPanel();
    QWidget* createThreadDumpPanel();

    void updateHealthPanel(const QJsonObject& data);
    void updateMetricsPanel(const QJsonObject& data);
    void updateInfoPanel(const QJsonObject& data);
    void updateEnvPanel(const QJsonObject& data);
    void updateThreadDumpPanel(const QJsonObject& data);

    QString formatJson(const QJsonObject& obj) const;
    void setTableFromJson(QTableWidget* table, const QJsonObject& obj);

    QTabWidget* m_tabWidget = nullptr;
    QTimer* m_refreshTimer = nullptr;
    int m_refreshInterval = 5000;

    // 健康面板
    QLabel* m_healthStatus = nullptr;
    QLabel* m_healthUptime = nullptr;
    QTableWidget* m_healthComponents = nullptr;

    // 指标面板
    QTableWidget* m_metricsTable = nullptr;

    // 信息面板
    QTextEdit* m_infoText = nullptr;

    // 环境面板
    QTableWidget* m_envTable = nullptr;

    // 线程转储面板
    QTextEdit* m_threadDumpText = nullptr;

    // 数据提供者
    std::function<QJsonObject()> m_healthProvider;
    std::function<QJsonObject()> m_metricsProvider;
    std::function<QJsonObject()> m_infoProvider;
    std::function<QJsonObject()> m_envProvider;
    std::function<QJsonObject()> m_threadDumpProvider;
};

} // namespace sc::cs

#endif // SOUL_CS_ADMIN_PANEL_H