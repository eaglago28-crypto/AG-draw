#include "RectShape.h"

#include <QPainter>

namespace agdraw::engine {

void RectShape::paint(QPainter &painter) const {
    const QRectF r = rect.normalized();

    if (shadowEnabled) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadowColor);
        painter.drawRect(r.translated(shadowOffset));
    }

    if (contourEnabled && contourSteps > 0) {
        painter.setPen(Qt::NoPen);
        for (int i = contourSteps; i >= 1; --i) {
            const qreal t = static_cast<qreal>(i) / contourSteps;
            const qreal grow = contourOffset * i;
            painter.setBrush(interpolateColor(fillColor, contourColor, t));
            painter.drawRect(r.adjusted(-grow, -grow, grow, grow));
        }
    }

    painter.setPen(QPen(strokeColor, strokeWidth));
    painter.setBrush(gradientEnabled ? QBrush(makeShapeGradient(r, gradientStartColor, gradientEndColor, gradientAngle))
                                      : QBrush(fillColor));
    painter.drawRect(r);
}

} // namespace agdraw::engine
