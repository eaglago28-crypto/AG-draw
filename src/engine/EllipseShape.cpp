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

    if (contourEnabled && contourSteps > 0) {
        painter.setPen(Qt::NoPen);
        for (int i = contourSteps; i >= 1; --i) {
            const qreal t = static_cast<qreal>(i) / contourSteps;
            const qreal grow = contourOffset * i;
            painter.setBrush(interpolateColor(fillColor, contourColor, t));
            painter.drawEllipse(r.adjusted(-grow, -grow, grow, grow));
        }
    }

    painter.setPen(QPen(strokeColor, strokeWidth));
    painter.setBrush(gradientEnabled ? QBrush(makeShapeGradient(r, gradientStartColor, gradientEndColor, gradientAngle))
                                      : QBrush(fillColor));
    painter.drawEllipse(r);
}

} // namespace agdraw::engine
