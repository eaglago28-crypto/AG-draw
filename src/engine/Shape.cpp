#include "Shape.h"

#include <QtMath>
#include <algorithm>

namespace agdraw::engine {

QLinearGradient makeShapeGradient(const QRectF &bounds, const QColor &start, const QColor &end, qreal angleDegrees) {
    const qreal radians = qDegreesToRadians(angleDegrees);
    const QPointF center = bounds.center();
    const qreal halfDiagonal = std::hypot(bounds.width(), bounds.height()) / 2.0;
    const QPointF direction(std::cos(radians), std::sin(radians));

    QLinearGradient gradient(center - direction * halfDiagonal, center + direction * halfDiagonal);
    gradient.setColorAt(0.0, start);
    gradient.setColorAt(1.0, end);
    return gradient;
}

QColor interpolateColor(const QColor &a, const QColor &b, qreal t) {
    t = std::clamp(t, 0.0, 1.0);
    return QColor::fromRgbF(a.redF() + (b.redF() - a.redF()) * t, a.greenF() + (b.greenF() - a.greenF()) * t,
                             a.blueF() + (b.blueF() - a.blueF()) * t, a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

QVector<QPointF> defaultEnvelopeCorners(const QRectF &bounds) {
    return {bounds.topLeft(), bounds.topRight(), bounds.bottomRight(), bounds.bottomLeft()};
}

QPolygonF applyEnvelope(const QPolygonF &source, const QRectF &sourceBounds, const QVector<QPointF> &corners) {
    if (corners.size() != 4) {
        return source;
    }
    const qreal w = sourceBounds.width();
    const qreal h = sourceBounds.height();

    QPolygonF result;
    result.reserve(source.size());
    for (const QPointF &p : source) {
        const qreal u = w > 0.0 ? (p.x() - sourceBounds.left()) / w : 0.0;
        const qreal v = h > 0.0 ? (p.y() - sourceBounds.top()) / h : 0.0;
        const QPointF warped = (1 - u) * (1 - v) * corners[0] + u * (1 - v) * corners[1] + u * v * corners[2] +
                                (1 - u) * v * corners[3];
        result.append(warped);
    }
    return result;
}

} // namespace agdraw::engine
