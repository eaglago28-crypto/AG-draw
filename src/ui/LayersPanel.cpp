#include "LayersPanel.h"

#include "Document.h"

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
        if (!m_document) {
            return;
        }
        m_document->addLayer(tr("Calque %1").arg(static_cast<int>(m_document->layers().size()) + 1));
        refresh();
    });
    layout->addWidget(toolbar);

    m_list = new QListWidget(container);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (!m_document || row < 0) {
            return;
        }
        const auto &layers = m_document->layers();
        // La liste affiche le calque le plus récent en haut.
        const int index = static_cast<int>(layers.size()) - 1 - row;
        if (index >= 0 && index < static_cast<int>(layers.size())) {
            m_document->setActiveLayer(layers[static_cast<size_t>(index)].get());
        }
    });

    setWidget(container);
}

void LayersPanel::setDocument(engine::Document *document) {
    m_document = document;
    refresh();
}

void LayersPanel::refresh() {
    m_list->clear();
    if (!m_document) {
        return;
    }
    const auto &layers = m_document->layers();
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        m_list->addItem((*it)->name());
    }
    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
}

} // namespace agdraw::ui
