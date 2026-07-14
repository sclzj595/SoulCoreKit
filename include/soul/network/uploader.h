#ifndef SOUL_NETWORK_UPLOADER_H
#define SOUL_NETWORK_UPLOADER_H

#include <QObject>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

namespace sc {

class Uploader : public QObject {
    Q_OBJECT
public:
    using ProgressCallback = std::function<void(qint64 uploaded, qint64 total)>;
    using CompleteCallback = std::function<void(bool success, const QString& errorMessage)>;

    explicit Uploader(QObject* parent = nullptr);
    ~Uploader() override;

    void upload(const QString& filePath, const QUrl& uploadUrl);
    void uploadMultipart(const QString& filePath, const QUrl& uploadUrl, const QString& fieldName = "file");
    void pause();
    void resume();
    void cancel();

    bool isRunning() const;
    bool isPaused() const;
    qint64 uploadedBytes() const;
    qint64 totalBytes() const;

    void setProgressCallback(ProgressCallback callback);
    void setCompleteCallback(CompleteCallback callback);

    void setTimeout(int ms);
    int timeout() const;

    void setChunkSize(qint64 size);
    qint64 chunkSize() const;

private slots:
    void onUploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void onUploadFinished();
    void onError(QNetworkReply::NetworkError error);

private:
    void startUpload();
    void uploadNextChunk();

    QNetworkAccessManager* m_manager = nullptr;
    QNetworkReply* m_reply = nullptr;
    QFile m_file;
    QUrl m_uploadUrl;
    QString m_filePath;
    QString m_fieldName;

    qint64 m_uploadedBytes = 0;
    qint64 m_totalBytes = 0;
    qint64 m_chunkSize = 1024 * 1024;
    bool m_isPaused = false;
    bool m_isRunning = false;
    int m_timeout = 30000;

    ProgressCallback m_progressCallback;
    CompleteCallback m_completeCallback;
};

}

#endif