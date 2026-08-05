#include "DocumentItem.h"

#include "Document.h"
#include "Page.h"

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

    for (const auto &page : m_document.pages()) {
        const QRectF rect = page->rect();
        painter->fillRect(rect, Qt::white);
        painter->setPen(QPen(QColor(120, 120, 130), 0));
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect);

        QFont labelFont = painter->font();
        labelFont.setPointSizeF(10);
        painter->setFont(labelFont);
        painter->setPen(QColor(190, 190, 200));
        painter->drawText(rect.topLeft() + QPointF(0, -6), page->name());
    }

    m_document.paint(*painter);
}

} // namespace agdraw::ui
