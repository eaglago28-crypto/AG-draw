#include "RectShape.h"

#include <QPainter>

namespace agdraw::engine {

void RectShape::paint(QPainter &painter) const {
    painter.setBrush(fillColor);
    painter.setPen(QPen(strokeColor, strokeWidth));
    painter.drawRect(rect.normalized());
}

} // namespace agdraw::engine
