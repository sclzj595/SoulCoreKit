#ifndef SOUL_UTILS_XML_XML_UTILS_H
#define SOUL_UTILS_XML_XML_UTILS_H

#include <QDomDocument>
#include <QString>
#include <QVariant>

namespace sc {

class XmlUtils {
public:
    static QDomDocument parse(const QString& xml);
    static QString serialize(const QDomDocument& doc, bool formatted = true);

    static QDomElement createElement(QDomDocument& doc, const QString& tagName,
                                     const QString& text = "");
    static QDomElement createElement(QDomDocument& doc, const QString& tagName,
                                     const QMap<QString, QString>& attributes,
                                     const QString& text = "");

    static QString getText(const QDomElement& element);
    static QString getAttribute(const QDomElement& element, const QString& name,
                                const QString& defaultValue = "");

    static QDomElement findElement(const QDomElement& root, const QString& tagName);
    static QList<QDomElement> findElements(const QDomElement& root, const QString& tagName);

    static bool saveToFile(const QDomDocument& doc, const QString& path);
    static QDomDocument loadFromFile(const QString& path);

    static QVariant toVariant(const QDomElement& element);
    static QDomElement fromVariant(QDomDocument& doc, const QString& tagName, const QVariant& value);
};

}

#endif