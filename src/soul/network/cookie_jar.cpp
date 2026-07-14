#include "soul/network/cookie_jar.h"
#include "soul/logging/log_macros.h"
#include <QFile>
#include <QDataStream>

namespace sc {

CookieJar::CookieJar(QObject* parent) : QNetworkCookieJar(parent) {}

void CookieJar::setPersistPath(const QString& path) {
    m_persistPath = path;
}

QString CookieJar::persistPath() const {
    return m_persistPath;
}

bool CookieJar::load() {
    if (m_persistPath.isEmpty()) {
        return false;
    }

    QFile file(m_persistPath);
    if (!file.open(QIODevice::ReadOnly)) {
        SC_WARN("Cannot open cookie file for reading: " + m_persistPath.toStdString());
        return false;
    }

    QByteArray data = file.readAll();
    QList<QNetworkCookie> cookies = QNetworkCookie::parseCookies(data);
    setAllCookies(cookies);

    SC_INFO("Loaded " + QString::number(cookies.size()).toStdString() + " cookies");
    return true;
}

bool CookieJar::save() const {
    if (m_persistPath.isEmpty()) {
        return false;
    }

    QList<QNetworkCookie> cookies = allCookies();
    QByteArray data;

    for (const QNetworkCookie& cookie : cookies) {
        if (!cookie.isSessionCookie()) {
            data += cookie.toRawForm(QNetworkCookie::Full);
            data += "\n";
        }
    }

    QFile file(m_persistPath);
    if (!file.open(QIODevice::WriteOnly)) {
        SC_WARN("Cannot open cookie file for writing: " + m_persistPath.toStdString());
        return false;
    }

    file.write(data);
    file.close();

    SC_INFO("Saved " + QString::number(cookies.size()).toStdString() + " cookies");
    return true;
}

void CookieJar::clearExpired() {
    QList<QNetworkCookie> cookies = allCookies();
    QList<QNetworkCookie> validCookies;

    for (const QNetworkCookie& cookie : cookies) {
        if (!cookie.isSessionCookie() && cookie.expirationDate() < QDateTime::currentDateTime()) {
            continue;
        }
        validCookies.append(cookie);
    }

    setAllCookies(validCookies);
}

void CookieJar::clearAll() {
    setAllCookies({});
}

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl& url) const {
    return QNetworkCookieJar::cookiesForUrl(url);
}

bool CookieJar::setCookiesFromUrl(const QList<QNetworkCookie>& cookies, const QUrl& url) {
    return QNetworkCookieJar::setCookiesFromUrl(cookies, url);
}

}