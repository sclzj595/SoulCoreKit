#include "soul/base/base_object.h"

namespace sc {

BaseObject::BaseObject(QObject* parent) : QObject(parent) {}

BaseObject::BaseObject(const QString& debugName, QObject* parent)
    : QObject(parent), m_debugName(debugName) {}

QString BaseObject::debugName() const {
    return m_debugName.isEmpty() ? objectName() : m_debugName;
}

void BaseObject::setDebugName(const QString& name) {
    m_debugName = name;
}

QString BaseObject::objectPath() const {
    QString path = debugName();
    QObject* parent = this->parent();
    while (parent) {
        auto* baseObj = dynamic_cast<BaseObject*>(parent);
        QString parentName = baseObj ? baseObj->debugName() : parent->objectName();
        path = parentName + "/" + path;
        parent = parent->parent();
    }
    return path;
}

}