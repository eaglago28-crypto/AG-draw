#pragma once

#include "Shape.h"

namespace agdraw::engine {

class EllipseShape : public Shape {
public:
    explicit EllipseShape(const QRectF &rect) : rect(rect) {}

    QRectF bounds() const override { return rect.normalized(); }
    bool contains(const QPointF &point) const override;
    void translate(const QPointF &delta) override { rect.translate(delta); }
    void setBounds(const QRectF &newRect) override { rect = newRect; }
    void paint(QPainter &painter) const override;

    QRectF rect;
};

} // namespace agdraw::engine
