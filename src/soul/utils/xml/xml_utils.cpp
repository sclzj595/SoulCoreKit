#include "soul/utils/xml/xml_utils.h"
#include <QFile>

namespace sc {

QDomDocument XmlUtils::parse(const QString& xml) {
    QDomDocument doc;
    doc.setContent(xml);
    return doc;
}

QString XmlUtils::serialize(const QDomDocument& doc, bool formatted) {
    if (formatted) {
        return doc.toString(4);
    }
    return doc.toString(0);
}

QDomElement XmlUtils::createElement(QDomDocument& doc, const QString& tagName, const QString& text) {
    QDomElement element = doc.createElement(tagName);
    if (!text.isEmpty()) {
        element.appendChild(doc.createTextNode(text));
    }
    return element;
}

QDomElement XmlUtils::createElement(QDomDocument& doc, const QString& tagName,
                                     const QMap<QString, QString>& attributes, const QString& text) {
    QDomElement element = doc.createElement(tagName);
    for (auto it = attributes.begin(); it != attributes.end(); ++it) {
        element.setAttribute(it.key(), it.value());
    }
    if (!text.isEmpty()) {
        element.appendChild(doc.createTextNode(text));
    }
    return element;
}

QString XmlUtils::getText(const QDomElement& element) {
    return element.text();
}

QString XmlUtils::getAttribute(const QDomElement& element, const QString& name, const QString& defaultValue) {
    return element.attribute(name, defaultValue);
}

QDomElement XmlUtils::findElement(const QDomElement& root, const QString& tagName) {
    return root.firstChildElement(tagName);
}

QList<QDomElement> XmlUtils::findElements(const QDomElement& root, const QString& tagName) {
    QList<QDomElement> elements;
    QDomElement element = root.firstChildElement(tagName);
    while (!element.isNull()) {
        elements.append(element);
        element = element.nextSiblingElement(tagName);
    }
    return elements;
}

bool XmlUtils::saveToFile(const QDomDocument& doc, const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    QTextStream stream(&file);
    doc.save(stream, 4);
    file.close();
    return true;
}

QDomDocument XmlUtils::loadFromFile(const QString& path) {
    QDomDocument doc;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        doc.setContent(&file);
        file.close();
    }
    return doc;
}

QVariant XmlUtils::toVariant(const QDomElement& element) {
    Q_UNUSED(element);
    return QVariant();
}

QDomElement XmlUtils::fromVariant(QDomDocument& doc, const QString& tagName, const QVariant& value) {
    Q_UNUSED(doc);
    Q_UNUSED(tagName);
    Q_UNUSED(value);
    return QDomElement();
}

}