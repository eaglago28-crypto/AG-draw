#include "LayersPanel.h"

#include "Document.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QToolBar>
#include <QToolButton>
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
        emit documentChanged();
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

    // La liste affiche le calque le plus récent en haut (ordre inverse).
    const auto &layers = m_document->layers();
    for (auto it = layers.rbegin(); it != layers.rend(); ++it) {
        engine::Layer *layer = it->get();

        auto *item = new QListWidgetItem(m_list);
        m_list->addItem(item);

        auto *row = new QWidget(m_list);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 2, 4, 2);

        auto *visibleButton = new QToolButton(row);
        visibleButton->setCheckable(true);
        visibleButton->setChecked(layer->isVisible());
        visibleButton->setToolTip(tr("Visibilité du calque"));
        visibleButton->setText(layer->isVisible() ? QStringLiteral("\U0001F441") : QStringLiteral("—"));
        connect(visibleButton, &QToolButton::toggled, this, [this, layer, visibleButton](bool checked) {
            layer->setVisible(checked);
            visibleButton->setText(checked ? QStringLiteral("\U0001F441") : QStringLiteral("—"));
            emit documentChanged();
        });

        auto *lockButton = new QToolButton(row);
        lockButton->setCheckable(true);
        lockButton->setChecked(layer->isLocked());
        lockButton->setToolTip(tr("Verrouiller le calque"));
        lockButton->setText(layer->isLocked() ? QStringLiteral("\U0001F512") : QStringLiteral("\U0001F513"));
        connect(lockButton, &QToolButton::toggled, this, [this, layer, lockButton](bool checked) {
            layer->setLocked(checked);
            lockButton->setText(checked ? QStringLiteral("\U0001F512") : QStringLiteral("\U0001F513"));
            emit documentChanged();
        });

        auto *nameLabel = new QLabel(layer->name(), row);

        rowLayout->addWidget(visibleButton);
        rowLayout->addWidget(lockButton);
        rowLayout->addWidget(nameLabel, 1);

        item->setSizeHint(row->sizeHint());
        m_list->setItemWidget(item, row);
    }

    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }
}

} // namespace agdraw::ui
