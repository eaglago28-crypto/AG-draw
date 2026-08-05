#pragma once

#include <QDockWidget>

class QListWidget;

namespace agdraw::engine {
class Document;
class Page;
}

namespace agdraw::ui {

// Panneau des pages à droite : liste les pages du document (rectangles
// nommés dans l'espace de dessin), permet d'en ajouter/supprimer et de
// naviguer vers l'une d'elles.
class PagesPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit PagesPanel(QWidget *parent = nullptr);

    void setDocument(agdraw::engine::Document *document);

signals:
    void documentChanged();
    void pageActivated(agdraw::engine::Page *page);

private:
    void refresh();

    agdraw::engine::Document *m_document = nullptr;
    QListWidget *m_list;
};

} // namespace agdraw::ui
