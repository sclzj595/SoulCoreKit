#include "soul/ui/base_view_model.h"
#include <mutex>

namespace sc {
namespace ui {

BaseViewModel::BaseViewModel(QObject* parent) : QObject(parent) {}

QVariant BaseViewModel::get(const QString& key, const QVariant& defaultValue) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_properties.find(key);
    if (it != m_properties.end()) {
        return it->second;
    }
    return defaultValue;
}

void BaseViewModel::set(const QString& key, const QVariant& value) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_properties[key] == value) {
            return;
        }
        m_properties[key] = value;
    }
    emit propertyChanged(key);
}


void BaseViewModel::setLoading(bool isLoading) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_isLoading == isLoading) {
            return;
        }
        m_isLoading = isLoading;
    }
    emit loadingChanged(isLoading);
}

void BaseViewModel::emitError(const QString& message) {
    emit errorOccurred(message);
}

} // namespace ui
} // namespace sc
