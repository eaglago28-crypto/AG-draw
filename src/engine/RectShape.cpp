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

    painter.setPen(QPen(strokeColor, strokeWidth));
    painter.setBrush(gradientEnabled ? QBrush(makeShapeGradient(r, gradientStartColor, gradientEndColor, gradientAngle))
                                      : QBrush(fillColor));
    painter.drawRect(r);
}

} // namespace agdraw::engine
