#ifndef SOUL_BASE_BASE_DIALOG_H
#define SOUL_BASE_BASE_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include "soul/ui/theme.h"

namespace sc {

class BaseDialog : public QDialog {
    Q_OBJECT
public:
    explicit BaseDialog(QWidget* parent = nullptr);
    ~BaseDialog() override = default;

    void setDialogTitle(const QString& title);
    void addButton(const QString& text, std::function<void()> callback = nullptr);

protected:
    void applyTheme();
    void setupLayout();

private slots:
    void onThemeChanged();

private:
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_buttonLayout;
};

}

#endif