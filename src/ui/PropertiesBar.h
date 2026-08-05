#pragma once

#include <QToolBar>
#include "Arrange.h"
#include "ToolBox.h"

class QLabel;
class QDoubleSpinBox;
class QCheckBox;
class QToolButton;

namespace agdraw::engine {
class Shape;
}

namespace agdraw::ui {

// Barre des propriétés en haut : affiche les réglages de l'outil actif et,
// si une forme est sélectionnée, ses propriétés réelles (épaisseur de
// trait, ombre portée, dégradé, alignement/distribution pour une
// sélection multiple).
class PropertiesBar : public QToolBar {
    Q_OBJECT

public:
    explicit PropertiesBar(QWidget *parent = nullptr);

public slots:
    void setActiveTool(agdraw::ui::Tool tool);
    void setSelectedShape(agdraw::engine::Shape *shape);
    void setSelectionCount(int count);

signals:
    void strokeWidthEdited(double value);
    void shadowToggled(bool enabled);
    void gradientToggled(bool enabled);
    void contourToggled(bool enabled);
    void alignRequested(agdraw::ui::AlignMode mode);
    void distributeRequested(agdraw::ui::DistributeMode mode);

private:
    QToolButton *addAlignButton(const QString &text, const QString &tooltip, AlignMode mode);
    QToolButton *addDistributeButton(const QString &text, const QString &tooltip, DistributeMode mode);

    QLabel *m_toolLabel;
    QDoubleSpinBox *m_strokeWidth;
    QCheckBox *m_shadowCheck;
    QCheckBox *m_gradientCheck;
    QCheckBox *m_contourCheck;
    QList<QToolButton *> m_alignButtons;
    QList<QToolButton *> m_distributeButtons;
};

} // namespace agdraw::ui
