#pragma once

#include "Shape.h"

namespace agdraw::engine {

class RectShape : public Shape {
public:
    explicit RectShape(const QRectF &rect) : rect(rect) {}

    QRectF bounds() const override { return rect.normalized(); }
    bool contains(const QPointF &point) const override { return bounds().contains(point); }
    void translate(const QPointF &delta) override { rect.translate(delta); }
    void setBounds(const QRectF &newRect) override { rect = newRect; }
    bool isResizable() const override { return true; }
    void paint(QPainter &painter) const override;

    QRectF rect;
};

} // namespace agdraw::engine
