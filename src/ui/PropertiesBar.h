#pragma once

#include <QToolBar>
#include "ToolBox.h"

class QLabel;
class QDoubleSpinBox;

namespace agdraw::ui {

// Barre des propriétés en haut : affiche les réglages de l'outil actif.
class PropertiesBar : public QToolBar {
    Q_OBJECT

public:
    explicit PropertiesBar(QWidget *parent = nullptr);

public slots:
    void setActiveTool(agdraw::ui::Tool tool);

private:
    QLabel *m_toolLabel;
    QDoubleSpinBox *m_strokeWidth;
};

} // namespace agdraw::ui
