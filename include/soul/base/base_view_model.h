#ifndef SOUL_BASE_BASE_VIEW_MODEL_H
#define SOUL_BASE_BASE_VIEW_MODEL_H

#include <QObject>
#include <QVariant>
#include <map>
#include <functional>

namespace sc {

class BaseViewModel : public QObject {
    Q_OBJECT
public:
    explicit BaseViewModel(QObject* parent = nullptr);
    ~BaseViewModel() override = default;

    QVariant get(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void set(const QString& key, const QVariant& value);

    template<typename T>
    T getValue(const QString& key, const T& defaultValue = T()) const {
        auto it = m_properties.find(key);
        if (it != m_properties.end()) {
            return it->second.value<T>();
        }
        return defaultValue;
    }

    template<typename T>
    void setValue(const QString& key, const T& value) {
        if (m_properties[key] != value) {
            m_properties[key] = value;
            emit propertyChanged(key);
        }
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
};

}

#endif