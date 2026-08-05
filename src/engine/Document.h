#pragma once

#include "Layer.h"
#include "Page.h"

#include <QUndoStack>
#include <memory>
#include <vector>

class QPainter;

namespace agdraw::engine {

// Modèle du document : indépendant de tout moteur de rendu (QPainter
// aujourd'hui, Skia demain). Source de vérité pour l'IU et l'export.
//
// Les calques (et leurs formes) et les pages sont deux listes
// indépendantes : les pages ne possèdent pas leurs propres calques, ce
// sont simplement des rectangles nommés dans le même espace de dessin
// continu (comme dans CorelDRAW), utilisés pour la mise en page et
// l'export/impression.
class Document {
public:
    Document();

    Layer &addLayer(const QString &name);
    Layer *activeLayer();
    void setActiveLayer(Layer *layer);
    void clearLayers();

    const std::vector<std::unique_ptr<Layer>> &layers() const { return m_layers; }

    Shape *shapeAt(const QPointF &point) const;
    Layer *findLayerOf(Shape *shape) const;
    void paint(QPainter &painter) const;

    Page &addPage(const QString &name, const QRectF &rect);
    Page *addPage(std::unique_ptr<Page> page);
    Page *insertPage(size_t index, std::unique_ptr<Page> page);
    std::unique_ptr<Page> takePage(Page *page);
    size_t indexOfPage(Page *page) const;
    Page *activePage();
    void setActivePage(Page *page);
    void clearPages();

    const std::vector<std::unique_ptr<Page>> &pages() const { return m_pages; }

    QUndoStack *undoStack() { return &m_undoStack; }

private:
    std::vector<std::unique_ptr<Layer>> m_layers;
    Layer *m_activeLayer = nullptr;

    std::vector<std::unique_ptr<Page>> m_pages;
    Page *m_activePage = nullptr;

    QUndoStack m_undoStack;
};

} // namespace agdraw::engine
