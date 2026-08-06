#include "CompoundPathShape.h"

#include <QPainter>

namespace agdraw::engine {

CompoundPathShape::CompoundPathShape(std::vector<QVector<QPointF>> contours) : m_contours(std::move(contours)) {}

QPainterPath CompoundPathShape::toPath() const {
    QPainterPath path;
    for (const QVector<QPointF> &contour : m_contours) {
        if (contour.isEmpty()) {
            continue;
        }
        path.moveTo(contour.first());
        for (int i = 1; i < contour.size(); ++i) {
            path.lineTo(contour[i]);
        }
        path.closeSubpath();
    }
    return path;
}

QRectF CompoundPathShape::bounds() const {
    return toPath().boundingRect();
}

bool CompoundPathShape::contains(const QPointF &point) const {
    return toPath().contains(point);
}

void CompoundPathShape::translate(const QPointF &delta) {
    for (QVector<QPointF> &contour : m_contours) {
        for (QPointF &point : contour) {
            point += delta;
        }
    }
}

void CompoundPathShape::setBounds(const QRectF &rect) {
    const QRectF oldBounds = bounds();
    if (oldBounds.width() <= 0.0 || oldBounds.height() <= 0.0) {
        return;
    }
    const qreal sx = rect.width() / oldBounds.width();
    const qreal sy = rect.height() / oldBounds.height();
    for (QVector<QPointF> &contour : m_contours) {
        for (QPointF &point : contour) {
            point = QPointF(rect.left() + (point.x() - oldBounds.left()) * sx,
                             rect.top() + (point.y() - oldBounds.top()) * sy);
        }
    }
}

void CompoundPathShape::paint(QPainter &painter) const {
    painter.setPen(strokeWidth > 0.0 ? QPen(strokeColor, strokeWidth) : Qt::NoPen);
    painter.setBrush(fillColor);
    painter.drawPath(toPath());
}

} // namespace agdraw::engine
