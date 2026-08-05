#include "PagesPanel.h"

#include "Commands.h"
#include "Document.h"
#include "Page.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace agdraw::ui {

namespace {
constexpr qreal kPageGap = 60.0;
}

PagesPanel::PagesPanel(QWidget *parent) : QDockWidget(tr("Pages"), parent) {
    setObjectName("PagesPanel");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *toolbar = new QToolBar(container);
    QAction *addAction = toolbar->addAction(tr("+ Page"));
    connect(addAction, &QAction::triggered, this, [this] {
        if (!m_document) {
            return;
        }
        QRectF newRect(0, 0, 794, 1123);
        if (!m_document->pages().empty()) {
            newRect = m_document->pages().back()->rect();
            newRect.moveTop(newRect.bottom() + kPageGap);
        }
        auto page = std::make_unique<engine::Page>(tr("Page %1").arg(m_document->pages().size() + 1), newRect);
        auto *command = new engine::AddPageCommand(m_document, std::move(page), tr("Page"));
        m_document->undoStack()->push(command);
        refresh();
        emit documentChanged();
        emit pageActivated(command->pagePtr());
    });
    layout->addWidget(toolbar);

    m_list = new QListWidget(container);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::currentRowChanged, this, [this](int row) {
        if (!m_document || row < 0) {
            return;
        }
        const auto &pages = m_document->pages();
        if (row < static_cast<int>(pages.size())) {
            engine::Page *page = pages[static_cast<size_t>(row)].get();
            m_document->setActivePage(page);
            emit pageActivated(page);
        }
    });

    setWidget(container);
}

void PagesPanel::setDocument(engine::Document *document) {
    m_document = document;
    refresh();
}

void PagesPanel::refresh() {
    m_list->clear();
    if (!m_document) {
        return;
    }

    const auto &pages = m_document->pages();
    for (const auto &pagePtr : pages) {
        engine::Page *page = pagePtr.get();

        auto *item = new QListWidgetItem(m_list);
        m_list->addItem(item);

        auto *row = new QWidget(m_list);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 2, 4, 2);

        auto *nameLabel = new QLabel(page->name(), row);

        auto *deleteButton = new QToolButton(row);
        deleteButton->setText(QStringLiteral("✕"));
        deleteButton->setToolTip(tr("Supprimer la page"));
        deleteButton->setEnabled(pages.size() > 1);
        connect(deleteButton, &QToolButton::clicked, this, [this, page] {
            if (!m_document || m_document->pages().size() <= 1) {
                return;
            }
            m_document->undoStack()->push(new engine::RemovePageCommand(m_document, page, tr("Supprimer la page")));
            refresh();
            emit documentChanged();
        });

        rowLayout->addWidget(nameLabel, 1);
        rowLayout->addWidget(deleteButton);

        item->setSizeHint(row->sizeHint());
        m_list->setItemWidget(item, row);
    }

    // Sélectionne la ligne de la page active sans redéclencher de
    // navigation (pageActivated) à chaque rafraîchissement.
    const QSignalBlocker blocker(m_list);
    engine::Page *active = m_document->activePage();
    for (int i = 0; i < static_cast<int>(pages.size()); ++i) {
        if (pages[static_cast<size_t>(i)].get() == active) {
            m_list->setCurrentRow(i);
            break;
        }
    }
}

} // namespace agdraw::ui
