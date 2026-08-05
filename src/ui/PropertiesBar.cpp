#include "PropertiesBar.h"

#include "Shape.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>

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
    m_strokeWidth->setEnabled(false);
    connect(m_strokeWidth, &QDoubleSpinBox::valueChanged, this, &PropertiesBar::strokeWidthEdited);
    addWidget(m_strokeWidth);

    addSeparator();

    m_shadowCheck = new QCheckBox(tr("Ombre portée"), this);
    m_shadowCheck->setEnabled(false);
    connect(m_shadowCheck, &QCheckBox::toggled, this, &PropertiesBar::shadowToggled);
    addWidget(m_shadowCheck);

    m_gradientCheck = new QCheckBox(tr("Dégradé"), this);
    m_gradientCheck->setEnabled(false);
    connect(m_gradientCheck, &QCheckBox::toggled, this, &PropertiesBar::gradientToggled);
    addWidget(m_gradientCheck);
}

void PropertiesBar::setActiveTool(Tool tool) {
    m_toolLabel->setText(toolName(tool));
}

void PropertiesBar::setSelectedShape(engine::Shape *shape) {
    const QSignalBlocker strokeBlocker(m_strokeWidth);
    const QSignalBlocker shadowBlocker(m_shadowCheck);
    const QSignalBlocker gradientBlocker(m_gradientCheck);

    if (shape) {
        m_strokeWidth->setEnabled(true);
        m_strokeWidth->setValue(shape->strokeWidth);

        const bool supportsEffects = shape->supportsFillEffects();
        m_shadowCheck->setEnabled(supportsEffects);
        m_shadowCheck->setChecked(shape->shadowEnabled);
        m_gradientCheck->setEnabled(supportsEffects);
        m_gradientCheck->setChecked(shape->gradientEnabled);
    } else {
        m_strokeWidth->setEnabled(false);
        m_shadowCheck->setEnabled(false);
        m_shadowCheck->setChecked(false);
        m_gradientCheck->setEnabled(false);
        m_gradientCheck->setChecked(false);
    }
}

} // namespace agdraw::ui
