#include "soul/utils/json/json_utils.h"
#include <QFile>

namespace sc {
namespace utils {
namespace json {

QJsonDocument fromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonDocument();
    }
    QByteArray data = file.readAll();
    return QJsonDocument::fromJson(data);
}

bool toFile(const QJsonDocument& doc, const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson());
    return true;
}

QString getString(const QJsonObject& obj, const QString& key, const QString& defaultValue) {
    if (obj.contains(key) && obj[key].isString()) {
        return obj[key].toString();
    }
    return defaultValue;
}

int getInt(const QJsonObject& obj, const QString& key, int defaultValue) {
    if (obj.contains(key) && obj[key].isDouble()) {
        return obj[key].toInt();
    }
    return defaultValue;
}

double getDouble(const QJsonObject& obj, const QString& key, double defaultValue) {
    if (obj.contains(key) && obj[key].isDouble()) {
        return obj[key].toDouble();
    }
    return defaultValue;
}

bool getBool(const QJsonObject& obj, const QString& key, bool defaultValue) {
    if (obj.contains(key) && obj[key].isBool()) {
        return obj[key].toBool();
    }
    return defaultValue;
}

QJsonArray getArray(const QJsonObject& obj, const QString& key) {
    if (obj.contains(key) && obj[key].isArray()) {
        return obj[key].toArray();
    }
    return QJsonArray();
}

QJsonObject getObject(const QJsonObject& obj, const QString& key) {
    if (obj.contains(key) && obj[key].isObject()) {
        return obj[key].toObject();
    }
    return QJsonObject();
}

bool contains(const QJsonObject& obj, const QString& key) {
    return obj.contains(key);
}

QString toPrettyString(const QJsonDocument& doc) {
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString toCompactString(const QJsonDocument& doc) {
    return QString::fromUtf8(doc.toJson(QJsonDocument::Compact));
}

} // namespace json
} // namespace utils
} // namespace sc
