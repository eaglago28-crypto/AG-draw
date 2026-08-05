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
    const QRectF r = rect.normalized();

    if (shadowEnabled) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadowColor);
        painter.drawEllipse(r.translated(shadowOffset));
    }

    painter.setPen(QPen(strokeColor, strokeWidth));
    painter.setBrush(gradientEnabled ? QBrush(makeShapeGradient(r, gradientStartColor, gradientEndColor, gradientAngle))
                                      : QBrush(fillColor));
    painter.drawEllipse(r);
}

} // namespace agdraw::engine
