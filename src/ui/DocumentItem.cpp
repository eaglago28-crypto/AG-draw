#include "DocumentItem.h"

#include "Document.h"

#include <QPainter>

namespace agdraw::ui {

DocumentItem::DocumentItem(agdraw::engine::Document &document, const QRectF &extent)
    : m_document(document), m_extent(extent) {
    setFlag(QGraphicsItem::ItemUsesExtendedStyleOption, true);
}

QRectF DocumentItem::boundingRect() const {
    return m_extent;
}

void DocumentItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    painter->setRenderHint(QPainter::Antialiasing);
    m_document.paint(*painter);
}

} // namespace agdraw::ui
