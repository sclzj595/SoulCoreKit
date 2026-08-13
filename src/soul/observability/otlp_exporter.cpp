#include "soul/observability/otlp_exporter.h"

namespace sc {
namespace observability {

// ============================================================================
// OtlpHttpExporter
// ============================================================================
OtlpHttpExporter& OtlpHttpExporter::instance() {
    static OtlpHttpExporter s_instance;
    return s_instance;
}

OtlpHttpExporter::~OtlpHttpExporter() {
    shutdown();
}

Result<void> OtlpHttpExporter::initialize(const OtlpConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
    m_networkManager = new QNetworkAccessManager(this);
    m_flushTimer = new QTimer(this);
    connect(m_flushTimer, &QTimer::timeout, this, &OtlpHttpExporter::onFlushTimer);
    m_flushTimer->start(m_config.flushIntervalMs);
    m_initialized = true;
    return Result<void>::ok();
}

void OtlpHttpExporter::shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_flushTimer) {
        m_flushTimer->stop();
    }
    m_initialized = false;
    m_traceBuffer.clear();
    // m_metricBuffer and m_logBuffer are QJsonArray, no explicit clear needed
    m_metricBuffer = QJsonArray();
    m_logBuffer = QJsonArray();
}

void OtlpHttpExporter::setConfig(const OtlpConfig& config) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_config = config;
}

void OtlpHttpExporter::setResourceAttributes(const ResourceAttributes& attrs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resourceAttrs = attrs;
}

void OtlpHttpExporter::exportSpan(std::shared_ptr<Span> span) {
    if (!span) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_traceBuffer.push_back(std::move(span));
    checkBufferSize();
}

void OtlpHttpExporter::exportSpans(const std::vector<std::shared_ptr<Span>>& spans) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& span : spans) {
        if (span) {
            m_traceBuffer.push_back(span);
        }
    }
    checkBufferSize();
}

void OtlpHttpExporter::flushTraces() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_traceBuffer.empty()) {
        QJsonObject payload = buildScopeSpansJson(m_traceBuffer);
        m_traceBuffer.clear();
        sendExport(m_config.tracesPath, payload, "traces");
    }
}

void OtlpHttpExporter::exportMetrics(const QJsonObject& metrics) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_metricBuffer.append(metrics);
    checkBufferSize();
}

void OtlpHttpExporter::exportLog(const QJsonObject& logRecord) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_logBuffer.append(logRecord);
    checkBufferSize();
}

void OtlpHttpExporter::exportLogs(const QJsonArray& logRecords) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (const auto& record : logRecords) {
        m_logBuffer.append(record);
    }
    checkBufferSize();
}

void OtlpHttpExporter::flush() {
    flushBuffer();
}

int OtlpHttpExporter::pendingTraces() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return static_cast<int>(m_traceBuffer.size());
}

int OtlpHttpExporter::pendingMetrics() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_metricBuffer.size();
}

int OtlpHttpExporter::pendingLogs() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_logBuffer.size();
}

void OtlpHttpExporter::onFlushTimer() {
    flushBuffer();
}

void OtlpHttpExporter::onExportFinished(QNetworkReply* reply) {
    if (!reply) return;
    reply->deleteLater();
}

QJsonObject OtlpHttpExporter::buildResourceJson() const {
    QJsonObject resource;
    QJsonArray attributes;

    if (!m_resourceAttrs.serviceName.isEmpty()) {
        QJsonObject attr;
        attr["key"] = "service.name";
        QJsonObject val;
        val["stringValue"] = m_resourceAttrs.serviceName;
        attr["value"] = val;
        attributes.append(attr);
    }

    resource["attributes"] = attributes;
    return resource;
}

QJsonObject OtlpHttpExporter::buildScopeSpansJson(const std::vector<std::shared_ptr<Span>>& spans) const {
    Q_UNUSED(spans);
    return QJsonObject();
}

QJsonObject OtlpHttpExporter::buildSpanJson(const Span& span) const {
    Q_UNUSED(span);
    return QJsonObject();
}

QJsonObject OtlpHttpExporter::buildScopeMetricsJson(const QJsonArray& metrics) const {
    Q_UNUSED(metrics);
    return QJsonObject();
}

QJsonObject OtlpHttpExporter::buildScopeLogsJson(const QJsonArray& logRecords) const {
    Q_UNUSED(logRecords);
    return QJsonObject();
}

void OtlpHttpExporter::sendExport(const QString& path, const QJsonObject& payload,
                                   const QString& signalType, int retryCount) {
    Q_UNUSED(path);
    Q_UNUSED(payload);
    Q_UNUSED(signalType);
    Q_UNUSED(retryCount);
}

void OtlpHttpExporter::flushBuffer() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_traceBuffer.empty()) {
        QJsonObject payload = buildScopeSpansJson(m_traceBuffer);
        m_traceBuffer.clear();
        sendExport(m_config.tracesPath, payload, "traces");
    }
    if (!m_metricBuffer.isEmpty()) {
        QJsonObject payload = buildScopeMetricsJson(m_metricBuffer);
        m_metricBuffer = QJsonArray();
        sendExport(m_config.metricsPath, payload, "metrics");
    }
    if (!m_logBuffer.isEmpty()) {
        QJsonObject payload = buildScopeLogsJson(m_logBuffer);
        m_logBuffer = QJsonArray();
        sendExport(m_config.logsPath, payload, "logs");
    }
}

void OtlpHttpExporter::checkBufferSize() {
    if (static_cast<int>(m_traceBuffer.size()) >= m_config.batchSize ||
        m_metricBuffer.size() >= m_config.batchSize ||
        m_logBuffer.size() >= m_config.batchSize) {
        // 不在这里直接 flush，由定时器或外部调用 flush
    }
}

} // namespace observability
} // namespace sc
