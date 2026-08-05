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

    if (envelopeEnabled && envelopeCorners.size() == 4) {
        constexpr int kSubdivisions = 24;
        QPolygonF outline;
        auto addEdge = [&](const QPointF &a, const QPointF &b) {
            for (int i = 0; i < kSubdivisions; ++i) {
                const qreal t = static_cast<qreal>(i) / kSubdivisions;
                outline.append(a + (b - a) * t);
            }
        };
        addEdge(r.topLeft(), r.topRight());
        addEdge(r.topRight(), r.bottomRight());
        addEdge(r.bottomRight(), r.bottomLeft());
        addEdge(r.bottomLeft(), r.topLeft());

        painter.setPen(QPen(strokeColor, strokeWidth));
        painter.setBrush(gradientEnabled
                              ? QBrush(makeShapeGradient(r, gradientStartColor, gradientEndColor, gradientAngle))
                              : QBrush(fillColor));
        painter.drawPolygon(applyEnvelope(outline, r, envelopeCorners));
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
