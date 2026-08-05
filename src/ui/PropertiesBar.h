#pragma once

#include <QToolBar>
#include "ToolBox.h"

class QLabel;
class QDoubleSpinBox;

namespace agdraw::engine {
class Shape;
}

namespace agdraw::ui {

// Barre des propriétés en haut : affiche les réglages de l'outil actif et,
// si une forme unique est sélectionnée, son épaisseur de trait réelle.
class PropertiesBar : public QToolBar {
    Q_OBJECT

public:
    explicit PropertiesBar(QWidget *parent = nullptr);

public slots:
    void setActiveTool(agdraw::ui::Tool tool);
    void setSelectedShape(agdraw::engine::Shape *shape);

signals:
    void strokeWidthEdited(double value);

private:
    QLabel *m_toolLabel;
    QDoubleSpinBox *m_strokeWidth;
};

} // namespace agdraw::ui
