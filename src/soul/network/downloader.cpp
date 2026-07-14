#include "soul/network/downloader.h"
#include "soul/logging/log_macros.h"
#include <QDir>

namespace sc {

Downloader::Downloader(QObject* parent) : QObject(parent) {
    m_manager = new QNetworkAccessManager(this);
}

Downloader::~Downloader() {
    cancel();
}

void Downloader::download(const QUrl& url, const QString& savePath) {
    m_url = url;
    m_savePath = savePath;
    m_downloadedBytes = 0;
    m_totalBytes = 0;
    m_isPaused = false;
    m_isRunning = true;

    startDownload();
}

void Downloader::pause() {
    if (m_reply && m_isRunning && !m_isPaused) {
        m_reply->abort();
        m_isPaused = true;
        SC_INFO("Download paused");
    }
}

void Downloader::resume() {
    if (m_isPaused) {
        m_isPaused = false;
        startDownload();
        SC_INFO("Download resumed");
    }
}

void Downloader::cancel() {
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
    SC_INFO("Download cancelled");
}

bool Downloader::isRunning() const {
    return m_isRunning;
}

bool Downloader::isPaused() const {
    return m_isPaused;
}

qint64 Downloader::downloadedBytes() const {
    return m_downloadedBytes;
}

qint64 Downloader::totalBytes() const {
    return m_totalBytes;
}

void Downloader::setProgressCallback(ProgressCallback callback) {
    m_progressCallback = std::move(callback);
}

void Downloader::setCompleteCallback(CompleteCallback callback) {
    m_completeCallback = std::move(callback);
}

void Downloader::setTimeout(int ms) {
    m_timeout = ms;
}

int Downloader::timeout() const {
    return m_timeout;
}

void Downloader::onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    m_downloadedBytes = bytesReceived;
    m_totalBytes = bytesTotal;

    if (m_progressCallback) {
        m_progressCallback(m_downloadedBytes, m_totalBytes);
    }
}

void Downloader::onDownloadFinished() {
    if (!m_reply) return;

    if (m_reply->error() == QNetworkReply::NoError) {
        m_file.write(m_reply->readAll());
        m_file.close();

        m_isRunning = false;
        SC_INFO("Download completed: " + m_savePath.toStdString());

        if (m_completeCallback) {
            m_completeCallback(true, "");
        }
    } else {
        if (m_reply->error() != QNetworkReply::OperationCanceledError) {
            SC_ERROR("Download failed: " + m_reply->errorString().toStdString());
            if (m_completeCallback) {
                m_completeCallback(false, m_reply->errorString());
            }
        }
    }

    m_reply->deleteLater();
    m_reply = nullptr;
}

void Downloader::onReadyRead() {
    if (m_reply && m_file.isOpen()) {
        m_file.write(m_reply->readAll());
    }
}

void Downloader::onError(QNetworkReply::NetworkError error) {
    SC_ERROR("Download error: " + QString::number(error).toStdString() + " - " + m_reply->errorString().toStdString());
}

void Downloader::startDownload() {
    qint64 existingSize = getExistingFileSize(m_savePath);

    QString dir = QFileInfo(m_savePath).absolutePath();
    if (!QDir().mkpath(dir)) {
        if (m_completeCallback) {
            m_completeCallback(false, "Cannot create directory");
        }
        return;
    }

    m_file.setFileName(m_savePath);
    if (existingSize > 0) {
        if (!m_file.open(QIODevice::Append)) {
            if (m_completeCallback) {
                m_completeCallback(false, "Cannot open file for append");
            }
            return;
        }
        m_downloadedBytes = existingSize;
    } else {
        if (!m_file.open(QIODevice::WriteOnly)) {
            if (m_completeCallback) {
                m_completeCallback(false, "Cannot open file for writing");
            }
            return;
        }
    }

    QNetworkRequest request(m_url);
    if (existingSize > 0) {
        request.setRawHeader("Range", QString("bytes=%1-").arg(existingSize).toUtf8());
    }

    m_reply = m_manager->get(request);

    connect(m_reply, &QNetworkReply::downloadProgress, this, &Downloader::onDownloadProgress);
    connect(m_reply, &QNetworkReply::finished, this, &Downloader::onDownloadFinished);
    connect(m_reply, &QNetworkReply::readyRead, this, &Downloader::onReadyRead);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &Downloader::onError);
}

qint64 Downloader::getExistingFileSize(const QString& path) {
    QFile file(path);
    if (file.exists()) {
        return file.size();
    }
    return 0;
}

}