#include <QApplication>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QTimer>
#include "soul/ui/toast.h"
#include "soul/ui/dialog.h"
#include "soul/ui/theme.h"
#include "soul/ui/base_widget.h"
#include "soul/ui/base_dialog.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setOrganizationName("SoulCore");
    app.setApplicationName("SoulCoreDemo");

    sc::Theme::instance().style().setColor(sc::ColorRole::Primary, QColor("#4CAF50"));

    sc::ui::BaseWidget window;
    window.setWindowTitle("SoulCore Demo");
    window.resize(800, 600);

    QVBoxLayout* layout = new QVBoxLayout(&window);

    QPushButton* toastButton = new QPushButton("Show Toast", &window);
    QPushButton* dialogButton = new QPushButton("Show Dialog", &window);
    QPushButton* themeButton = new QPushButton("Toggle Theme", &window);

    layout->addWidget(toastButton);
    layout->addWidget(dialogButton);
    layout->addWidget(themeButton);

    QObject::connect(toastButton, &QPushButton::clicked, []() {
        sc::Toast toast;
        toast.setMessage("Hello from SoulCore!");
        toast.show();
    });

    QObject::connect(dialogButton, &QPushButton::clicked, []() {
        sc::Dialog dialog;
        dialog.setTitle("Information");
        dialog.setMessage("This is a dialog message");
        dialog.show();
    });

    QObject::connect(themeButton, &QPushButton::clicked, []() {
        auto& theme = sc::Theme::instance();
        if (theme.style().color(sc::ColorRole::Primary) == QColor("#4CAF50")) {
            theme.style().setColor(sc::ColorRole::Primary, QColor("#2196F3"));
        } else {
            theme.style().setColor(sc::ColorRole::Primary, QColor("#4CAF50"));
        }
    });

    window.show();

    return app.exec();
}
