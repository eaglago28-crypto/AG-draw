#include "ToolBox.h"

#include <QActionGroup>

namespace agdraw::ui {

ToolBox::ToolBox(QWidget *parent) : QToolBar(tr("Outils"), parent) {
    setObjectName("ToolBox");
    setOrientation(Qt::Vertical);
    setMovable(false);
    setIconSize(QSize(24, 24));

    auto *group = new QActionGroup(this);
    group->setExclusive(true);

    addTool(tr("Sélection"), Tool::Selection, group)->setChecked(true);
    addTool(tr("Rectangle"), Tool::Rectangle, group);
    addTool(tr("Ellipse"), Tool::Ellipse, group);
    addTool(tr("Texte"), Tool::Text, group);
    addTool(tr("Plume"), Tool::Pen, group);
    addTool(tr("Pinceau"), Tool::Brush, group);
}

QAction *ToolBox::addTool(const QString &text, Tool tool, QActionGroup *group) {
    auto *action = addAction(text);
    action->setCheckable(true);
    action->setActionGroup(group);
    connect(action, &QAction::triggered, this, [this, tool] { emit toolSelected(tool); });
    return action;
}

} // namespace agdraw::ui
