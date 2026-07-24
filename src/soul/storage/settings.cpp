#include "soul/storage/settings.h"
#include <QStandardPaths>

namespace sc {

Settings::Settings() : QSettings(QSettings::IniFormat, QSettings::UserScope, "SoulCore", "SoulCore") {}

void Settings::setPath(const QString& organization, const QString& application) {
    QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    path += "/" + organization + "/" + application;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, path);
}

bool Settings::contains(const QString& key) const {
    return QSettings::contains(key);
}

void Settings::remove(const QString& key) {
    QSettings::remove(key);
    sync();
}

void Settings::clear() {
    QSettings::clear();
    sync();
}

void Settings::sync() {
    QSettings::sync();
}

}