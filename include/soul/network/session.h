#ifndef SOUL_NETWORK_SESSION_H
#define SOUL_NETWORK_SESSION_H

#include "soul/network/network_global.h"
#ifndef Q_MOC_RUN
#include "soul/core/singleton.h"
#endif
#include <QString>
#include <QMap>
#include <memory>
#include <mutex>

namespace sc {
namespace network {

class SC_NETWORK_EXPORT Session : public Singleton<Session> {
    friend class Singleton<Session>;
public:
    QString token() const;
    void setToken(const QString& token);
    QString get(const QString& key) const;
    void set(const QString& key, const QString& value);
    void clear();
    bool isEmpty() const;

private:
    Session() = default;
    ~Session() = default;

    mutable std::mutex m_mutex;
    QString m_token;
    QMap<QString, QString> m_data;
};

} // namespace network
} // namespace sc

#endif