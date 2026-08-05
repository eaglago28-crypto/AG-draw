#include "TextShape.h"

#include <QFont>
#include <QFontMetricsF>
#include <QPainter>

namespace agdraw::engine {

namespace {
QFont makeFont(qreal pointSize) {
    QFont font;
    font.setPointSizeF(pointSize);
    return font;
}
} // namespace

QRectF TextShape::bounds() const {
    QFontMetricsF metrics(makeFont(fontPointSize));
    return metrics.boundingRect(text).translated(position);
}

bool TextShape::contains(const QPointF &point) const {
    return bounds().contains(point);
}

void TextShape::paint(QPainter &painter) const {
    painter.setFont(makeFont(fontPointSize));
    painter.setPen(fillColor);
    painter.drawText(position, text);
}

} // namespace agdraw::engine
