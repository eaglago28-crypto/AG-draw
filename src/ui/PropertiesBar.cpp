#include "PropertiesBar.h"

#include "Shape.h"

#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QToolButton>

namespace agdraw::ui {

namespace {
QString toolName(Tool tool) {
    switch (tool) {
        case Tool::Selection: return QObject::tr("Sélection");
        case Tool::Rectangle: return QObject::tr("Rectangle");
        case Tool::Ellipse: return QObject::tr("Ellipse");
        case Tool::Text: return QObject::tr("Texte");
        case Tool::Pen: return QObject::tr("Plume");
        case Tool::Brush: return QObject::tr("Pinceau");
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

    m_contourCheck = new QCheckBox(tr("Contour"), this);
    m_contourCheck->setEnabled(false);
    connect(m_contourCheck, &QCheckBox::toggled, this, &PropertiesBar::contourToggled);
    addWidget(m_contourCheck);

    addSeparator();

    addAlignButton(QStringLiteral("⟸"), tr("Aligner à gauche"), AlignMode::Left);
    addAlignButton(QStringLiteral("⟺"), tr("Centrer horizontalement"), AlignMode::HCenter);
    addAlignButton(QStringLiteral("⟹"), tr("Aligner à droite"), AlignMode::Right);
    addAlignButton(QStringLiteral("⤒"), tr("Aligner en haut"), AlignMode::Top);
    addAlignButton(QStringLiteral("⇳"), tr("Centrer verticalement"), AlignMode::VCenter);
    addAlignButton(QStringLiteral("⤓"), tr("Aligner en bas"), AlignMode::Bottom);

    addSeparator();

    addDistributeButton(QStringLiteral("⇔"), tr("Distribuer horizontalement"), DistributeMode::Horizontal);
    addDistributeButton(QStringLiteral("⇕"), tr("Distribuer verticalement"), DistributeMode::Vertical);
}

QToolButton *PropertiesBar::addAlignButton(const QString &text, const QString &tooltip, AlignMode mode) {
    auto *button = new QToolButton(this);
    button->setText(text);
    button->setToolTip(tooltip);
    button->setEnabled(false);
    connect(button, &QToolButton::clicked, this, [this, mode] { emit alignRequested(mode); });
    addWidget(button);
    m_alignButtons.append(button);
    return button;
}

QToolButton *PropertiesBar::addDistributeButton(const QString &text, const QString &tooltip, DistributeMode mode) {
    auto *button = new QToolButton(this);
    button->setText(text);
    button->setToolTip(tooltip);
    button->setEnabled(false);
    connect(button, &QToolButton::clicked, this, [this, mode] { emit distributeRequested(mode); });
    addWidget(button);
    m_distributeButtons.append(button);
    return button;
}

void PropertiesBar::setActiveTool(Tool tool) {
    m_toolLabel->setText(toolName(tool));
}

void PropertiesBar::setSelectedShape(engine::Shape *shape) {
    const QSignalBlocker strokeBlocker(m_strokeWidth);
    const QSignalBlocker shadowBlocker(m_shadowCheck);
    const QSignalBlocker gradientBlocker(m_gradientCheck);
    const QSignalBlocker contourBlocker(m_contourCheck);

    if (shape) {
        m_strokeWidth->setEnabled(true);
        m_strokeWidth->setValue(shape->strokeWidth);

        const bool supportsEffects = shape->supportsFillEffects();
        m_shadowCheck->setEnabled(supportsEffects);
        m_shadowCheck->setChecked(shape->shadowEnabled);
        m_gradientCheck->setEnabled(supportsEffects);
        m_gradientCheck->setChecked(shape->gradientEnabled);
        m_contourCheck->setEnabled(supportsEffects);
        m_contourCheck->setChecked(shape->contourEnabled);
    } else {
        m_strokeWidth->setEnabled(false);
        m_shadowCheck->setEnabled(false);
        m_shadowCheck->setChecked(false);
        m_gradientCheck->setEnabled(false);
        m_gradientCheck->setChecked(false);
        m_contourCheck->setEnabled(false);
        m_contourCheck->setChecked(false);
    }
}

void PropertiesBar::setSelectionCount(int count) {
    for (QToolButton *button : m_alignButtons) {
        button->setEnabled(count >= 2);
    }
    for (QToolButton *button : m_distributeButtons) {
        button->setEnabled(count >= 3);
    }
}

} // namespace agdraw::ui
