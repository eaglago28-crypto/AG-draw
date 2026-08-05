#include "EllipseShape.h"

#include <QPainter>
#include <QPainterPath>

namespace agdraw::engine {

bool EllipseShape::contains(const QPointF &point) const {
    QPainterPath path;
    path.addEllipse(rect.normalized());
    return path.contains(point);
}

void EllipseShape::paint(QPainter &painter) const {
    painter.setBrush(fillColor);
    painter.setPen(QPen(strokeColor, strokeWidth));
    painter.drawEllipse(rect.normalized());
}

} // namespace agdraw::engine
