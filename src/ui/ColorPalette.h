#pragma once

#include <QToolBar>
#include <QColor>

namespace agdraw::ui {

// Palette de couleurs : bande verticale de nuances sélectionnables.
class ColorPalette : public QToolBar {
    Q_OBJECT

public:
    explicit ColorPalette(QWidget *parent = nullptr);

signals:
    void colorSelected(const QColor &color);

private:
    void addSwatch(const QColor &color);
};

} // namespace agdraw::ui
