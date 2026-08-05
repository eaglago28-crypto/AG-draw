#include "PathShape.h"

#include <QPainter>
#include <QPainterPathStroker>
#include <algorithm>

namespace agdraw::engine {

QPainterPath PathShape::toPath() const {
    QPainterPath path;
    if (points.isEmpty()) {
        return path;
    }
    path.moveTo(points.first());
    for (int i = 1; i < points.size(); ++i) {
        path.lineTo(points[i]);
    }
    return path;
}

QRectF PathShape::bounds() const {
    return toPath().boundingRect();
}

bool PathShape::contains(const QPointF &point) const {
    QPainterPathStroker stroker;
    stroker.setWidth(std::max(strokeWidth, 6.0));
    return stroker.createStroke(toPath()).contains(point);
}

void PathShape::translate(const QPointF &delta) {
    for (QPointF &point : points) {
        point += delta;
    }
}

void PathShape::paint(QPainter &painter) const {
    painter.setPen(QPen(strokeColor, strokeWidth));
    painter.setBrush(Qt::NoBrush);
    painter.drawPath(toPath());
}

} // namespace agdraw::engine
