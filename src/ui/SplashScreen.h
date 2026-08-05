#pragma once

#include <QSplashScreen>

namespace agdraw::ui {

// Écran de démarrage affiché pendant l'initialisation de l'application.
class SplashScreen : public QSplashScreen {
public:
    SplashScreen();

private:
    static QPixmap buildPixmap();
};

} // namespace agdraw::ui
