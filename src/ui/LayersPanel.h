#pragma once

#include <QDockWidget>

class QListWidget;

namespace agdraw::engine {
class Document;
}

namespace agdraw::ui {

// Panneau des calques à droite : reflète le vrai agdraw::engine::Document
// (liste, ajout, sélection du calque actif, visibilité, verrouillage).
class LayersPanel : public QDockWidget {
    Q_OBJECT

public:
    explicit LayersPanel(QWidget *parent = nullptr);

    void setDocument(agdraw::engine::Document *document);

signals:
    // Émis quand une propriété de calque change (visibilité, verrouillage) :
    // la zone de dessin doit se redessiner.
    void documentChanged();

private:
    void refresh();

    agdraw::engine::Document *m_document = nullptr;
    QListWidget *m_list;
};

} // namespace agdraw::ui
