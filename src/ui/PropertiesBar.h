#pragma once

#include <QToolBar>
#include "ToolBox.h"

class QLabel;
class QDoubleSpinBox;
class QCheckBox;

namespace agdraw::engine {
class Shape;
}

namespace agdraw::ui {

// Barre des propriétés en haut : affiche les réglages de l'outil actif et,
// si une forme est sélectionnée, ses propriétés réelles (épaisseur de
// trait, ombre portée, dégradé).
class PropertiesBar : public QToolBar {
    Q_OBJECT

public:
    explicit PropertiesBar(QWidget *parent = nullptr);

public slots:
    void setActiveTool(agdraw::ui::Tool tool);
    void setSelectedShape(agdraw::engine::Shape *shape);

signals:
    void strokeWidthEdited(double value);
    void shadowToggled(bool enabled);
    void gradientToggled(bool enabled);

private:
    QLabel *m_toolLabel;
    QDoubleSpinBox *m_strokeWidth;
    QCheckBox *m_shadowCheck;
    QCheckBox *m_gradientCheck;
};

} // namespace agdraw::ui
