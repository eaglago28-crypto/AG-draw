#include "SplashScreen.h"

#include <QPainter>

namespace agdraw::ui {

SplashScreen::SplashScreen() : QSplashScreen(buildPixmap()) {}

QPixmap SplashScreen::buildPixmap() {
    QPixmap pixmap(480, 300);
    pixmap.fill(QColor(32, 32, 36));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.setPen(Qt::white);
    QFont titleFont = painter.font();
    titleFont.setPointSize(28);
    titleFont.setBold(true);
    painter.setFont(titleFont);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, "AG Draw");

    QFont subFont = painter.font();
    subFont.setPointSize(10);
    subFont.setBold(false);
    painter.setFont(subFont);
    painter.setPen(QColor(180, 180, 190));
    painter.drawText(QRect(0, pixmap.height() - 40, pixmap.width(), 30),
                      Qt::AlignHCenter | Qt::AlignTop, "Chargement...");

    return pixmap;
}

} // namespace agdraw::ui
