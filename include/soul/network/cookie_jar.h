#ifndef SOUL_NETWORK_COOKIE_JAR_H
#define SOUL_NETWORK_COOKIE_JAR_H

#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QString>
#include <QDateTime>
#include <memory>

namespace sc {

class CookieJar : public QNetworkCookieJar {
    Q_OBJECT
public:
    explicit CookieJar(QObject* parent = nullptr);
    ~CookieJar() override = default;

    void setPersistPath(const QString& path);
    QString persistPath() const;

    bool load();
    bool save() const;

    void clearExpired();
    void clearAll();

    QList<QNetworkCookie> cookiesForUrl(const QUrl& url) const override;
    bool setCookiesFromUrl(const QList<QNetworkCookie>& cookies, const QUrl& url) override;

private:
    QString m_persistPath;
};

}

#endif