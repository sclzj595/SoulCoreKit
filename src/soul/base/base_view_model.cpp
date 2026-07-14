#include "soul/base/base_view_model.h"

namespace sc {

BaseViewModel::BaseViewModel(QObject* parent) : QObject(parent) {}

QVariant BaseViewModel::get(const QString& key, const QVariant& defaultValue) const {
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        return it->second;
    }
    return defaultValue;
}

void BaseViewModel::set(const QString& key, const QVariant& value) {
    if (m_properties[key] != value) {
        m_properties[key] = value;
        emit propertyChanged(key);
    }
}

void BaseViewModel::setLoading(bool isLoading) {
    if (m_isLoading != isLoading) {
        m_isLoading = isLoading;
        emit loadingChanged(isLoading);
    }
}

void BaseViewModel::emitError(const QString& message) {
    emit errorOccurred(message);
}

}