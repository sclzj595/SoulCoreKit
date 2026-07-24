#ifndef SOUL_STORAGE_SETTINGS_H
#define SOUL_STORAGE_SETTINGS_H

#include <QSettings>
#include <QString>
#include <QVariant>
#include <memory>
#include "soul/core/singleton.h"
#include "soul/core/result.h"

namespace sc {

class Settings : public QSettings, public Singleton<Settings> {
    Q_OBJECT
    friend class Singleton<Settings>;
public:
    ~Settings() override = default;

    void setPath(const QString& organization, const QString& application);

    template<typename T>
    T value(const QString& key, const T& defaultValue = T()) const {
        return QSettings::value(key, defaultValue).template value<T>();
    }

    template<typename T>
    void setValue(const QString& key, const T& value) {
        QSettings::setValue(key, value);
        sync();
        emit valueChanged(key);
    }

    bool contains(const QString& key) const;
    void remove(const QString& key);
    void clear();
    void sync();

signals:
    void valueChanged(const QString& key);

private:
    explicit Settings();
};

}

#endif