#include "soul/network/uploader.h"
#include "soul/logging/log_macros.h"
#include <QHttpMultiPart>
#include <QFileInfo>

namespace sc {
namespace network {

Uploader::Uploader(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
}

Uploader::~Uploader() {
    cancel();
}

void Uploader::upload(const QString& filePath, const QUrl& uploadUrl) {
    m_filePath = filePath;
    m_uploadUrl = uploadUrl;
    m_fieldName = "file";
    m_uploadedBytes = 0;
    m_isPaused = false;
    m_isRunning = true;

    startUpload();
}

void Uploader::uploadMultipart(const QString& filePath, const QUrl& uploadUrl, const QString& fieldName) {
    m_filePath = filePath;
    m_uploadUrl = uploadUrl;
    m_fieldName = fieldName;
    m_uploadedBytes = 0;
    m_isPaused = false;
    m_isRunning = true;

    startUpload();
}

void Uploader::pause() {
    if (m_reply && m_isRunning && !m_isPaused) {
        m_reply->abort();
        m_isPaused = true;
        SC_INFO("Upload paused");
    }
}

void Uploader::resume() {
    if (m_isPaused) {
        m_isPaused = false;
        startUpload();
        SC_INFO("Upload resumed");
    }
}

void Uploader::cancel() {
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_isRunning = false;
    m_isPaused = false;
    SC_INFO("Upload cancelled");
}

bool Uploader::isRunning() const {
    return m_isRunning;
}

bool Uploader::isPaused() const {
    return m_isPaused;
}

qint64 Uploader::uploadedBytes() const {
    return m_uploadedBytes;
}

qint64 Uploader::totalBytes() const {
    return m_totalBytes;
}

void Uploader::setProgressCallback(ProgressCallback callback) {
    m_progressCallback = std::move(callback);
}

void Uploader::setCompleteCallback(CompleteCallback callback) {
    m_completeCallback = std::move(callback);
}

void Uploader::setTimeout(int ms) {
    m_timeout = ms;
}

int Uploader::timeout() const {
    return m_timeout;
}

void Uploader::setChunkSize(qint64 size) {
    m_chunkSize = size;
}

qint64 Uploader::chunkSize() const {
    return m_chunkSize;
}

void Uploader::onUploadProgress(qint64 bytesSent, qint64 bytesTotal) {
    m_uploadedBytes = bytesSent;
    m_totalBytes = bytesTotal;

    if (m_progressCallback) {
        m_progressCallback(m_uploadedBytes, m_totalBytes);
    }
}

void Uploader::onUploadFinished() {
    if (!m_reply) return;

    if (m_reply->error() == QNetworkReply::NoError) {
        m_isRunning = false;
        SC_INFO("Upload completed: " + m_filePath.toStdString());

        if (m_completeCallback) {
            m_completeCallback(true, "");
        }
    } else {
        if (m_reply->error() != QNetworkReply::OperationCanceledError) {
            SC_ERROR("Upload failed: " + m_reply->errorString().toStdString());
            if (m_completeCallback) {
                m_completeCallback(false, m_reply->errorString());
            }
        }
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

void Uploader::onError(QNetworkReply::NetworkError error) {
    SC_ERROR("Upload error: " + QString::number(error).toStdString() + " - " + m_reply->errorString().toStdString());
}

void Uploader::startUpload() {
    m_file.setFileName(m_filePath);
    if (!m_file.exists()) {
        if (m_completeCallback) {
            m_completeCallback(false, "File not found");
        }
        return;
    }

    if (!m_file.open(QIODevice::ReadOnly)) {
        if (m_completeCallback) {
            m_completeCallback(false, "Cannot open file");
        }
        return;
    }

    m_totalBytes = m_file.size();

    auto multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QString("form-data; name=\"%1\"; filename=\"%2\"")
                       .arg(m_fieldName)
                       .arg(QFileInfo(m_filePath).fileName()));

    filePart.setBodyDevice(&m_file);
    m_file.setParent(multiPart);

    multiPart->append(filePart);

    QNetworkRequest request(m_uploadUrl);
    m_reply = m_manager->post(request, multiPart);
    multiPart->setParent(m_reply);

    connect(m_reply, &QNetworkReply::uploadProgress, this, &Uploader::onUploadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &Uploader::onUploadFinished);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &Uploader::onError);
}

void Uploader::uploadNextChunk() {
}

} // namespace network
} // namespace sc