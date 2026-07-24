#ifndef SOUL_UI_BASE_VIEW_MODEL_H
#define SOUL_UI_BASE_VIEW_MODEL_H

#include <QObject>
#include <QVariant>
#include <mutex>
#include <map>
#include <functional>

namespace sc {
namespace ui {


class BaseViewModel : public QObject {
    Q_OBJECT
public:
    explicit BaseViewModel(QObject* parent = nullptr);
    ~BaseViewModel() override = default;

    QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void set(const QString& key, const QVariant& value);

    template<typename T>
    T getValue(const QString& key, const T& defaultValue = T()) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_properties.find(key);
        if (it != m_properties.end()) {
            return it->second.value<T>();
        }
        return defaultValue;
    }

    template<typename T>
    void setValue(const QString& key, const T& value) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_properties[key] == value) {
                return;
            }
            m_properties[key] = value;
        }
        emit propertyChanged(key);
    }

signals:
    void propertyChanged(const QString& key);
    void errorOccurred(const QString& message);
    void loadingChanged(bool isLoading);

protected:
    void setLoading(bool isLoading);
    void emitError(const QString& message);

private:
    std::map<QString, QVariant> m_properties;
    bool m_isLoading = false;
    mutable std::mutex m_mutex;
};

} // namespace ui
} // namespace sc

#endif