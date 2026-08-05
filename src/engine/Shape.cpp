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

QPolygonF sampleRectOutline(const QRectF &rect, int subdivisionsPerEdge) {
    QPolygonF outline;
    auto addEdge = [&](const QPointF &a, const QPointF &b) {
        for (int i = 0; i < subdivisionsPerEdge; ++i) {
            const qreal t = static_cast<qreal>(i) / subdivisionsPerEdge;
            outline.append(a + (b - a) * t);
        }
    };
    addEdge(rect.topLeft(), rect.topRight());
    addEdge(rect.topRight(), rect.bottomRight());
    addEdge(rect.bottomRight(), rect.bottomLeft());
    addEdge(rect.bottomLeft(), rect.topLeft());
    return outline;
}

QPolygonF sampleEllipseOutline(const QRectF &rect, int samples) {
    const QPointF center = rect.center();
    const qreal rx = rect.width() / 2.0;
    const qreal ry = rect.height() / 2.0;

    QPolygonF outline;
    outline.reserve(samples);
    for (int i = 0; i < samples; ++i) {
        const qreal angle = 2.0 * M_PI * i / samples;
        outline.append(QPointF(center.x() + rx * std::cos(angle), center.y() + ry * std::sin(angle)));
    }
    return outline;
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
