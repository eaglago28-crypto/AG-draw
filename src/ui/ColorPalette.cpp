#include "ColorPalette.h"

#include <QToolButton>

namespace agdraw::ui {

namespace {
const QList<QColor> kDefaultPalette = {
    Qt::black, Qt::white, Qt::red, Qt::green, Qt::blue,
    Qt::yellow, Qt::cyan, Qt::magenta, Qt::gray, Qt::darkGray,
    QColor(255, 128, 0), QColor(128, 0, 255), QColor(0, 153, 76), QColor(153, 76, 0),
};
} // namespace

ColorPalette::ColorPalette(QWidget *parent) : QToolBar(tr("Couleurs"), parent) {
    setObjectName("ColorPalette");
    setOrientation(Qt::Vertical);
    setMovable(false);
    setIconSize(QSize(20, 20));

    for (const QColor &color : kDefaultPalette) {
        addSwatch(color);
    }
}

void ColorPalette::addSwatch(const QColor &color) {
    auto *button = new QToolButton(this);
    QPixmap pixmap(20, 20);
    pixmap.fill(color);
    button->setIcon(QIcon(pixmap));
    button->setToolTip(color.name());
    connect(button, &QToolButton::clicked, this, [this, color] { emit colorSelected(color); });
    addWidget(button);
}

} // namespace agdraw::ui
