#ifndef SOUL_UI_BASE_DIALOG_H
#define SOUL_UI_BASE_DIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <functional>
#include "soul/ui/theme.h"

namespace sc {
namespace ui {


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

} // namespace ui
} // namespace sc

#endif