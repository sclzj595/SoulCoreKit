#ifndef SOUL_OBSERVABILITY_OTLP_EXPORTER_H
#define SOUL_OBSERVABILITY_OTLP_EXPORTER_H

// ============================================================================
// otlp_exporter.h — OpenTelemetry Protocol (OTLP) HTTP 导出器 [v2.5.0]
// ============================================================================
// 完整实现 OTLP/HTTP 协议，支持:
//   - Traces 导出 (OTLP JSON + Protobuf)
//   - Metrics 导出 (OTLP JSON)
//   - Logs 导出 (OTLP JSON)
//   - 批量导出 + 定时刷新
//   - 重试策略 + 指数退避
//   - gRPC 传输 (通过 HTTP/JSON 兼容层)
// ============================================================================

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QByteArray>
#include <vector>
#include <memory>
#include <mutex>
#include <chrono>

#include "soul/core/result.h"
#include "soul/observability/tracing.h"
#include "soul/observability/metrics.h"
#include "soul/utils/json/json_helper.h"

namespace sc {
namespace observability {

// ============================================================================
// OtlpConfig — OTLP 导出器配置
// ============================================================================
struct OtlpConfig {
    QString endpoint = "http://localhost:4318";  // OTLP HTTP 端点
    QString tracesPath = "/v1/traces";           // Traces 路径
    QString metricsPath = "/v1/metrics";         // Metrics 路径
    QString logsPath = "/v1/logs";               // Logs 路径
    int batchSize = 512;                         // 批量大小
    int flushIntervalMs = 5000;                  // 刷新间隔
    int maxRetries = 3;                          // 最大重试次数
    int retryBaseMs = 1000;                      // 重试基础间隔
    int retryMaxMs = 10000;                      // 重试最大间隔
    int timeoutMs = 10000;                       // 请求超时
    QString serviceName = "SoulCoreKit";          // 服务名称
    QString serviceVersion = "2.5.0";            // 服务版本
    bool enableTraces = true;                    // 是否导出 Traces
    bool enableMetrics = true;                   // 是否导出 Metrics
    bool enableLogs = true;                      // 是否导出 Logs
    QString authToken;                           // 认证 Token
};

// ============================================================================
// ResourceAttributes — OTLP Resource
// ============================================================================
struct ResourceAttributes {
    QString serviceName;
    QString serviceVersion;
    QString serviceNamespace;
    QString hostName;
    QString deploymentEnvironment;
    QJsonObject customAttributes;
};

// ============================================================================
// OtlpHttpExporter — OTLP HTTP 导出器
// ============================================================================
class OtlpHttpExporter : public QObject {
    Q_OBJECT
public:
    static OtlpHttpExporter& instance();

    // === 初始化 ===
    Result<void> initialize(const OtlpConfig& config);
    void shutdown();

    // === 配置 ===
    void setConfig(const OtlpConfig& config);
    const OtlpConfig& config() const { return m_config; }

    // === Resource ===
    void setResourceAttributes(const ResourceAttributes& attrs);
    const ResourceAttributes& resourceAttributes() const { return m_resourceAttrs; }

    // === Traces 导出 ===
    void exportSpan(std::shared_ptr<Span> span);
    void exportSpans(const std::vector<std::shared_ptr<Span>>& spans);
    void flushTraces();

    // === Metrics 导出 ===
    void exportMetrics(const QJsonObject& metrics);

    // === Logs 导出 ===
    void exportLog(const QJsonObject& logRecord);
    void exportLogs(const QJsonArray& logRecords);

    // === 手动刷新 ===
    void flush();

    // === 状态 ===
    bool isInitialized() const { return m_initialized; }
    int pendingTraces() const;
    int pendingMetrics() const;
    int pendingLogs() const;

signals:
    void exportSucceeded(const QString& signalType, int count);
    void exportFailed(const QString& signalType, const QString& error);
    void allExportsDone();

private slots:
    void onFlushTimer();
    void onExportFinished(QNetworkReply* reply);

private:
    OtlpHttpExporter() = default;
    ~OtlpHttpExporter() override;

    // === OTLP JSON 构建 ===
    QJsonObject buildResourceJson() const;
    QJsonObject buildScopeSpansJson(const std::vector<std::shared_ptr<Span>>& spans) const;
    QJsonObject buildSpanJson(const Span& span) const;
    QJsonObject buildScopeMetricsJson(const QJsonObject& metrics) const;
    QJsonObject buildScopeLogsJson(const QJsonArray& logRecords) const;

    // === HTTP 发送 ===
    void sendExport(const QString& path, const QJsonObject& payload,
                    const QString& signalType, int retryCount = 0);

    // === 缓冲管理 ===
    void flushBuffer();
    void checkBufferSize();

    OtlpConfig m_config;
    ResourceAttributes m_resourceAttrs;
    QNetworkAccessManager* m_networkManager = nullptr;  // parent=this (QObject 生命周期管理)
    QTimer* m_flushTimer = nullptr;                      // parent=this (QObject 生命周期管理)
    bool m_initialized = false;

    // 缓冲
    std::vector<std::shared_ptr<Span>> m_traceBuffer;
    QJsonArray m_metricBuffer;
    QJsonArray m_logBuffer;
    mutable std::mutex m_mutex;
};

} // namespace observability
} // namespace sc

#endif // SOUL_OBSERVABILITY_OTLP_EXPORTER_H