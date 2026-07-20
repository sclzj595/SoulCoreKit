/**
 * @file cookie_jar.h
 * @brief Cookie 管理类
 * @details 继承 QNetworkCookieJar，提供 Cookie 的持久化和管理
 * @author SoulCoreKit Team
 * @date 2026-07-20
 * @version 1.0.0
 * @copyright MIT License
 */
#ifndef SOUL_NETWORK_COOKIE_JAR_H
#define SOUL_NETWORK_COOKIE_JAR_H

#include <QNetworkCookieJar>
#include <QNetworkCookie>
#include <QString>
#include <QDateTime>
#include <memory>
#include "soul/network/network_global.h"

namespace sc {
namespace network {

class SC_NETWORK_EXPORT CookieJar : public QNetworkCookieJar {
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

} // namespace network
} // namespace sc

#endif