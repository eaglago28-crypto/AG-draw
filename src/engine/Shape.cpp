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

} // namespace agdraw::engine
