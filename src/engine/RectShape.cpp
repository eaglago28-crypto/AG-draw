#include "RectShape.h"

#include <QPainter>
#include <QtMath>

namespace agdraw::engine {

void RectShape::paint(QPainter &painter) const {
    const QRectF r = rect.normalized();

    if (shadowEnabled) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(shadowColor);
        painter.drawRect(r.translated(shadowOffset));
    }

    if (envelopeEnabled && envelopeCorners.size() == 4) {
        painter.setPen(QPen(strokeColor, strokeWidth));
        painter.setBrush(gradientEnabled
                              ? QBrush(makeShapeGradient(r, gradientStartColor, gradientEndColor, gradientAngle))
                              : QBrush(fillColor));
        painter.drawPolygon(applyEnvelope(sampleRectOutline(r), r, envelopeCorners));
        return;
    }

    if (extrusionEnabled) {
        const qreal radians = qDegreesToRadians(extrusionAngle);
        const QPointF depth(extrusionDepth * std::cos(radians), extrusionDepth * std::sin(radians));
        const QPolygonF outline = sampleRectOutline(r);

        painter.setPen(Qt::NoPen);
        painter.setBrush(extrusionColor);
        for (int i = 0; i < outline.size(); ++i) {
            const QPointF &a = outline[i];
            const QPointF &b = outline[(i + 1) % outline.size()];
            painter.drawPolygon(QPolygonF{a, b, b + depth, a + depth});
        }

        painter.setPen(QPen(strokeColor, strokeWidth));
        painter.setBrush(gradientEnabled
                              ? QBrush(makeShapeGradient(r, gradientStartColor, gradientEndColor, gradientAngle))
                              : QBrush(fillColor));
        painter.drawRect(r);
        return;
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
