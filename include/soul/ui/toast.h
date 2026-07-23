#ifndef SOUL_UI_TOAST_H
#define SOUL_UI_TOAST_H

#include <QWidget>
#include <QString>
#include <QTimer>
#include <QVBoxLayout>
#include <memory>
#include <QPointer>
#include "soul/ui/design_constants.h"

namespace sc {

enum class ToastType {
    Info,
    Warning,
    Error,
    Success
};

class Toast : public QWidget {
    Q_OBJECT
public:
    explicit Toast(QWidget* parent = nullptr);
    ~Toast() override;

    void setType(ToastType type);
    void setMessage(const QString& message);
    void setDuration(int milliseconds);
    void show();

    static void info(const QString& message, int duration = design::TOAST_DEFAULT_DURATION);
    static void warning(const QString& message, int duration = design::TOAST_DEFAULT_DURATION);
    static void error(const QString& message, int duration = design::TOAST_DEFAULT_DURATION);
    static void success(const QString& message, int duration = design::TOAST_DEFAULT_DURATION);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();

    ToastType m_type;
    QString m_message;
    int m_duration;
    std::unique_ptr<QTimer> m_timer;
};

class ToastManager : public QWidget {
    Q_OBJECT
public:
    static ToastManager& instance();

    void addToast(std::unique_ptr<Toast> toast);

private:
    ToastManager();
    ~ToastManager() override;

    QVBoxLayout* m_layout;
};

}

#endif
