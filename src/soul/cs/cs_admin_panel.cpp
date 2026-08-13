// ============================================================================
// cs_admin_panel.cpp — 管理后台面板实现 [v2.5.0]
// ============================================================================

#include "soul/cs/cs_admin_panel.h"

#include <QGroupBox>
#include <QScrollArea>
#include <QHeaderView>
#include <QJsonArray>
#include <QDebug>

namespace sc::cs {

// ============================================================================
// CsAdminPanel 构造与析构
// ============================================================================

CsAdminPanel::CsAdminPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

CsAdminPanel::~CsAdminPanel() = default;

// ============================================================================
// 数据源配置
// ============================================================================

void CsAdminPanel::setHealthEndpoint(std::function<QJsonObject()> healthProvider) {
    m_healthProvider = std::move(healthProvider);
}

void CsAdminPanel::setMetricsEndpoint(std::function<QJsonObject()> metricsProvider) {
    m_metricsProvider = std::move(metricsProvider);
}

void CsAdminPanel::setInfoEndpoint(std::function<QJsonObject()> infoProvider) {
    m_infoProvider = std::move(infoProvider);
}

void CsAdminPanel::setEnvEndpoint(std::function<QJsonObject()> envProvider) {
    m_envProvider = std::move(envProvider);
}

void CsAdminPanel::setThreadDumpEndpoint(std::function<QJsonObject()> threadDumpProvider) {
    m_threadDumpProvider = std::move(threadDumpProvider);
}

// ============================================================================
// 刷新控制
// ============================================================================

void CsAdminPanel::setAutoRefreshInterval(int ms) {
    m_refreshInterval = ms;
    if (m_refreshTimer && m_refreshTimer->isActive()) {
        m_refreshTimer->setInterval(ms);
    }
}

void CsAdminPanel::startAutoRefresh() {
    if (!m_refreshTimer) {
        m_refreshTimer = new QTimer(this);
        QObject::connect(m_refreshTimer, &QTimer::timeout, this, &CsAdminPanel::refreshAll);
    }
    m_refreshTimer->start(m_refreshInterval);
}

void CsAdminPanel::stopAutoRefresh() {
    if (m_refreshTimer) {
        m_refreshTimer->stop();
    }
}

void CsAdminPanel::refreshAll() {
    onRefreshHealth();
    onRefreshMetrics();
    onRefreshInfo();
    onRefreshEnv();
    onRefreshThreadDump();
}

// ============================================================================
// UI 构建
// ============================================================================

void CsAdminPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createHealthPanel(),     QStringLiteral("Health"));
    m_tabWidget->addTab(createMetricsPanel(),    QStringLiteral("Metrics"));
    m_tabWidget->addTab(createInfoPanel(),       QStringLiteral("Info"));
    m_tabWidget->addTab(createEnvPanel(),         QStringLiteral("Env"));
    m_tabWidget->addTab(createThreadDumpPanel(), QStringLiteral("Thread Dump"));

    mainLayout->addWidget(m_tabWidget);
}

QWidget* CsAdminPanel::createHealthPanel() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    auto* headerGroup = new QGroupBox(QStringLiteral("Status"));
    auto* headerLayout = new QVBoxLayout(headerGroup);
    m_healthStatus = new QLabel(QStringLiteral("UNKNOWN"));
    m_healthUptime = new QLabel(QStringLiteral("Uptime: N/A"));
    headerLayout->addWidget(m_healthStatus);
    headerLayout->addWidget(m_healthUptime);
    layout->addWidget(headerGroup);

    auto* componentsGroup = new QGroupBox(QStringLiteral("Components"));
    auto* componentsLayout = new QVBoxLayout(componentsGroup);
    m_healthComponents = new QTableWidget(0, 3);
    m_healthComponents->setHorizontalHeaderLabels({QStringLiteral("Component"), QStringLiteral("Status"), QStringLiteral("Details")});
    m_healthComponents->horizontalHeader()->setStretchLastSection(true);
    componentsLayout->addWidget(m_healthComponents);
    layout->addWidget(componentsGroup);

    layout->addStretch();
    return page;
}

QWidget* CsAdminPanel::createMetricsPanel() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    m_metricsTable = new QTableWidget(0, 3);
    m_metricsTable->setHorizontalHeaderLabels({QStringLiteral("Metric"), QStringLiteral("Value"), QStringLiteral("Tags")});
    m_metricsTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_metricsTable);

    return page;
}

QWidget* CsAdminPanel::createInfoPanel() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    m_infoText = new QTextEdit();
    m_infoText->setReadOnly(true);
    layout->addWidget(m_infoText);

    return page;
}

QWidget* CsAdminPanel::createEnvPanel() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    m_envTable = new QTableWidget(0, 2);
    m_envTable->setHorizontalHeaderLabels({QStringLiteral("Property"), QStringLiteral("Value")});
    m_envTable->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(m_envTable);

    return page;
}

QWidget* CsAdminPanel::createThreadDumpPanel() {
    auto* page = new QWidget();
    auto* layout = new QVBoxLayout(page);

    m_threadDumpText = new QTextEdit();
    m_threadDumpText->setReadOnly(true);
    layout->addWidget(m_threadDumpText);

    return page;
}

// ============================================================================
// 数据更新
// ============================================================================

void CsAdminPanel::updateHealthPanel(const QJsonObject& jsonData) {
    if (m_healthStatus) {
        QString status = jsonData.value("status").toString("UNKNOWN");
        m_healthStatus->setText(status);
    }
    if (m_healthUptime) {
        QString uptime = jsonData.value("uptime").toString("N/A");
        m_healthUptime->setText(QStringLiteral("Uptime: %1").arg(uptime));
    }
    setTableFromJson(m_healthComponents, jsonData.value("components").toObject());
}

void CsAdminPanel::updateMetricsPanel(const QJsonObject& jsonData) {
    setTableFromJson(m_metricsTable, jsonData);
}

void CsAdminPanel::updateInfoPanel(const QJsonObject& jsonData) {
    if (m_infoText) {
        m_infoText->setPlainText(formatJson(jsonData));
    }
}

void CsAdminPanel::updateEnvPanel(const QJsonObject& jsonData) {
    setTableFromJson(m_envTable, jsonData);
}

void CsAdminPanel::updateThreadDumpPanel(const QJsonObject& jsonData) {
    if (m_threadDumpText) {
        m_threadDumpText->setPlainText(formatJson(jsonData));
    }
}

// ============================================================================
// 刷新槽
// ============================================================================

void CsAdminPanel::onRefreshHealth() {
    if (m_healthProvider) {
        updateHealthPanel(m_healthProvider());
    }
}

void CsAdminPanel::onRefreshMetrics() {
    if (m_metricsProvider) {
        updateMetricsPanel(m_metricsProvider());
    }
}

void CsAdminPanel::onRefreshInfo() {
    if (m_infoProvider) {
        updateInfoPanel(m_infoProvider());
    }
}

void CsAdminPanel::onRefreshEnv() {
    if (m_envProvider) {
        updateEnvPanel(m_envProvider());
    }
}

void CsAdminPanel::onRefreshThreadDump() {
    if (m_threadDumpProvider) {
        updateThreadDumpPanel(m_threadDumpProvider());
    }
}

// ============================================================================
// 工具方法
// ============================================================================

QString CsAdminPanel::formatJson(const QJsonObject& obj) const {
    QJsonDocument doc(obj);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

void CsAdminPanel::setTableFromJson(QTableWidget* table, const QJsonObject& obj) {
    if (!table) return;

    table->setRowCount(0);
    int row = 0;
    for (auto it = obj.begin(); it != obj.end(); ++it, ++row) {
        table->insertRow(row);
        table->setItem(row, 0, new QTableWidgetItem(it.key()));

        QJsonValue val = it.value();
        if (val.isObject()) {
            table->setItem(row, 1, new QTableWidgetItem(formatJson(val.toObject())));
        } else if (val.isArray()) {
            QJsonDocument doc(val.toArray());
            table->setItem(row, 1, new QTableWidgetItem(QString::fromUtf8(doc.toJson())));
        } else {
            table->setItem(row, 1, new QTableWidgetItem(val.toVariant().toString()));
        }
    }
}

} // namespace sc::cs
