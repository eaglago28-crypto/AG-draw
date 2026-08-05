#include "BrushStroke.h"

#include <QPainter>
#include <cmath>

namespace agdraw::engine {

QPainterPath buildBrushOutline(const QVector<BrushPoint> &points, qreal baseWidth) {
    QPainterPath path;
    if (points.size() < 2) {
        return path;
    }

    QVector<QPointF> left;
    QVector<QPointF> right;
    left.reserve(points.size());
    right.reserve(points.size());

    for (int i = 0; i < points.size(); ++i) {
        QPointF direction;
        if (i == 0) {
            direction = points[1].point - points[0].point;
        } else if (i == points.size() - 1) {
            direction = points[i].point - points[i - 1].point;
        } else {
            direction = points[i + 1].point - points[i - 1].point;
        }

        const qreal length = std::hypot(direction.x(), direction.y());
        const QPointF normal = length > 0.0 ? QPointF(-direction.y(), direction.x()) / length : QPointF(0, 0);
        const qreal halfWidth = std::max(baseWidth * points[i].pressure, 1.0) / 2.0;

        left.append(points[i].point + normal * halfWidth);
        right.append(points[i].point - normal * halfWidth);
    }

    path.moveTo(left.first());
    for (int i = 1; i < left.size(); ++i) {
        path.lineTo(left[i]);
    }
    for (int i = right.size() - 1; i >= 0; --i) {
        path.lineTo(right[i]);
    }
    path.closeSubpath();
    return path;
}

void BrushStroke::translate(const QPointF &delta) {
    for (BrushPoint &point : points) {
        point.point += delta;
    }
}

void BrushStroke::paint(QPainter &painter) const {
    painter.setPen(Qt::NoPen);
    painter.setBrush(strokeColor);
    painter.drawPath(buildBrushOutline(points, baseWidth));
}

} // namespace agdraw::engine
