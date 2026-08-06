#include "PathShape.h"

#include <QPainter>
#include <QPainterPathStroker>
#include <algorithm>

namespace agdraw::engine {

QPainterPath PathShape::toPath() const {
    QPainterPath path;
    if (nodes.isEmpty()) {
        return path;
    }
    path.moveTo(nodes.first().point);
    for (int i = 1; i < nodes.size(); ++i) {
        const PathNode &prev = nodes[i - 1];
        const PathNode &cur = nodes[i];
        if (prev.handle.isNull() && cur.handle.isNull()) {
            path.lineTo(cur.point);
        } else {
            path.cubicTo(prev.point + prev.handle, cur.point - cur.handle, cur.point);
        }
    }
    if (closed) {
        path.closeSubpath();
    }
    return path;
}

QRectF PathShape::bounds() const {
    return toPath().boundingRect();
}

bool PathShape::contains(const QPointF &point) const {
    const QPainterPath path = toPath();
    if (closed && path.contains(point)) {
        return true;
    }
    QPainterPathStroker stroker;
    stroker.setWidth(std::max(strokeWidth, 6.0));
    return stroker.createStroke(path).contains(point);
}

void PathShape::translate(const QPointF &delta) {
    for (PathNode &node : nodes) {
        node.point += delta;
    }
}

void PathShape::paint(QPainter &painter) const {
    if (closed) {
        painter.setPen(strokeWidth > 0.0 ? QPen(strokeColor, strokeWidth) : Qt::NoPen);
        painter.setBrush(fillColor);
    } else {
        painter.setPen(QPen(strokeColor, strokeWidth));
        painter.setBrush(Qt::NoBrush);
    }
    painter.drawPath(toPath());
}

} // namespace agdraw::engine
