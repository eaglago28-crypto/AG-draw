#pragma once

#include <QGraphicsItem>
#include <QRectF>

namespace agdraw::engine {
class Document;
}

namespace agdraw::ui {

// Adapte le modèle agdraw::engine::Document (indépendant de Qt Graphics View)
// à la scène QGraphicsScene utilisée pour le pan/zoom de la zone de dessin.
class DocumentItem : public QGraphicsItem {
public:
    DocumentItem(agdraw::engine::Document &document, const QRectF &extent);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

private:
    agdraw::engine::Document &m_document;
    QRectF m_extent;
};

} // namespace agdraw::ui
