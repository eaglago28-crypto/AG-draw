#include <QApplication>
#include <QTimer>

#include "MainWindow.h"
#include "SplashScreen.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("AG Draw");
    QApplication::setOrganizationName("AG Draw");

    agdraw::ui::SplashScreen splash;
    splash.show();
    app.processEvents();

    agdraw::ui::MainWindow window;

    QTimer::singleShot(1200, &splash, [&splash, &window] {
        window.show();
        splash.finish(&window);
    });

    return app.exec();
}
