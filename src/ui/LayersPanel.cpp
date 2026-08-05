#include "LayersPanel.h"

#include <QListWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <QWidget>

namespace agdraw::ui {

LayersPanel::LayersPanel(QWidget *parent) : QDockWidget(tr("Calques"), parent) {
    setObjectName("LayersPanel");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *toolbar = new QToolBar(container);
    QAction *addAction = toolbar->addAction(tr("+ Calque"));
    connect(addAction, &QAction::triggered, this, [this] {
        addLayer(tr("Calque %1").arg(++m_layerCount));
    });
    layout->addWidget(toolbar);

    m_list = new QListWidget(container);
    layout->addWidget(m_list);

    setWidget(container);
    addLayer(tr("Calque 1"));
}

void LayersPanel::addLayer(const QString &name) {
    m_list->insertItem(0, name);
    m_list->setCurrentRow(0);
}

} // namespace agdraw::ui
