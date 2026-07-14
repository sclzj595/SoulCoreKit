#ifndef SOUL_NETWORK_DOWNLOADER_H
#define SOUL_NETWORK_DOWNLOADER_H

#include <QObject>
#include <QUrl>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <functional>

namespace sc {

class Downloader : public QObject {
    Q_OBJECT
public:
    using ProgressCallback = std::function<void(qint64 downloaded, qint64 total)>;
    using CompleteCallback = std::function<void(bool success, const QString& errorMessage)>;

    explicit Downloader(QObject* parent = nullptr);
    ~Downloader() override;

    void download(const QUrl& url, const QString& savePath);
    void pause();
    void resume();
    void cancel();

    bool isRunning() const;
    bool isPaused() const;
    qint64 downloadedBytes() const;
    qint64 totalBytes() const;

    void setProgressCallback(ProgressCallback callback);
    void setCompleteCallback(CompleteCallback callback);

    void setTimeout(int ms);
    int timeout() const;

private slots:
    void onDownloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void onDownloadFinished();
    void onReadyRead();
    void onError(QNetworkReply::NetworkError error);

private:
    void startDownload();
    qint64 getExistingFileSize(const QString& path);

    QNetworkAccessManager* m_manager = nullptr;
    QNetworkReply* m_reply = nullptr;
    QFile m_file;
    QUrl m_url;
    QString m_savePath;

    qint64 m_downloadedBytes = 0;
    qint64 m_totalBytes = 0;
    bool m_isPaused = false;
    bool m_isRunning = false;
    int m_timeout = 30000;

    ProgressCallback m_progressCallback;
    CompleteCallback m_completeCallback;
};

}

#endif