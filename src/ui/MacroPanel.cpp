#include "MacroPanel.h"

#include "Document.h"
#include "Macro.h"

#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

namespace agdraw::ui {

MacroPanel::MacroPanel(QWidget *parent) : QDockWidget(tr("Macros"), parent) {
    setObjectName("MacroPanel");
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto *container = new QWidget(this);
    auto *layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);

    auto *toolbar = new QToolBar(container);
    m_recordButton = new QToolButton(toolbar);
    m_recordButton->setText(tr("● Enregistrer"));
    m_recordButton->setToolTip(tr("Enregistrer une nouvelle macro à partir des actions effectuées sur la sélection"));
    connect(m_recordButton, &QToolButton::clicked, this, [this] {
        if (!m_recording) {
            m_recording = true;
            m_recordButton->setText(tr("■ Arrêter"));
            emit recordingStartRequested();
        } else {
            m_recording = false;
            m_recordButton->setText(tr("● Enregistrer"));
            bool ok = false;
            const QString name =
                QInputDialog::getText(this, tr("Nom de la macro"), tr("Nom :"), QLineEdit::Normal, QString(), &ok);
            emit recordingStopRequested(ok ? name : QString());
            refresh();
        }
    });
    toolbar->addWidget(m_recordButton);
    layout->addWidget(toolbar);

    m_list = new QListWidget(container);
    layout->addWidget(m_list);

    setWidget(container);
}

void MacroPanel::setDocument(engine::Document *document) {
    m_document = document;
    refresh();
}

void MacroPanel::refresh() {
    m_list->clear();
    if (!m_document) {
        return;
    }

    for (const engine::Macro &macro : m_document->macros()) {
        auto *item = new QListWidgetItem(m_list);
        m_list->addItem(item);

        auto *row = new QWidget(m_list);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(4, 2, 4, 2);

        auto *nameLabel = new QLabel(tr("%1 (%2)").arg(macro.name).arg(macro.steps.size()), row);

        auto *playButton = new QToolButton(row);
        playButton->setText(QStringLiteral("▶"));
        playButton->setToolTip(tr("Rejouer sur la sélection courante"));
        connect(playButton, &QToolButton::clicked, this, [this, macro] { emit macroPlayRequested(macro); });

        auto *deleteButton = new QToolButton(row);
        deleteButton->setText(QStringLiteral("✕"));
        deleteButton->setToolTip(tr("Supprimer la macro"));
        connect(deleteButton, &QToolButton::clicked, this, [this, name = macro.name] {
            if (!m_document) {
                return;
            }
            m_document->removeMacro(name);
            refresh();
            emit documentChanged();
        });

        rowLayout->addWidget(nameLabel, 1);
        rowLayout->addWidget(playButton);
        rowLayout->addWidget(deleteButton);

        item->setSizeHint(row->sizeHint());
        m_list->setItemWidget(item, row);
    }
}

} // namespace agdraw::ui
