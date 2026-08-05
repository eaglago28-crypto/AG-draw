#include "PropertiesBar.h"

#include <QLabel>
#include <QDoubleSpinBox>

namespace agdraw::ui {

namespace {
QString toolName(Tool tool) {
    switch (tool) {
        case Tool::Selection: return QObject::tr("Sélection");
        case Tool::Rectangle: return QObject::tr("Rectangle");
        case Tool::Ellipse: return QObject::tr("Ellipse");
        case Tool::Text: return QObject::tr("Texte");
        case Tool::Pen: return QObject::tr("Plume");
    }
    return {};
}
} // namespace

PropertiesBar::PropertiesBar(QWidget *parent) : QToolBar(tr("Propriétés"), parent) {
    setObjectName("PropertiesBar");
    setMovable(false);

    m_toolLabel = new QLabel(toolName(Tool::Selection), this);
    addWidget(m_toolLabel);
    addSeparator();

    addWidget(new QLabel(tr("Épaisseur du trait :"), this));
    m_strokeWidth = new QDoubleSpinBox(this);
    m_strokeWidth->setRange(0.0, 100.0);
    m_strokeWidth->setSingleStep(0.5);
    m_strokeWidth->setValue(1.0);
    m_strokeWidth->setSuffix(tr(" pt"));
    addWidget(m_strokeWidth);
}

void PropertiesBar::setActiveTool(Tool tool) {
    m_toolLabel->setText(toolName(tool));
}

} // namespace agdraw::ui
