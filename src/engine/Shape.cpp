#include "Shape.h"

#include <QtMath>

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

} // namespace agdraw::engine
