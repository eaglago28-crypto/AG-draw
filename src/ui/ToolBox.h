#pragma once

#include <QToolBar>

class QActionGroup;

namespace agdraw::ui {

enum class Tool {
    Selection,
    Rectangle,
    Ellipse,
    Text,
    Pen,
};

// Barre d'outils verticale à gauche : sélection, rectangle, ellipse, texte, plume.
class ToolBox : public QToolBar {
    Q_OBJECT

public:
    explicit ToolBox(QWidget *parent = nullptr);

signals:
    void toolSelected(agdraw::ui::Tool tool);

private:
    QAction *addTool(const QString &text, Tool tool, QActionGroup *group);
};

} // namespace agdraw::ui
