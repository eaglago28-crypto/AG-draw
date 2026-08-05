#pragma once

#include "Shape.h"

#include <QPainterPath>
#include <QVector>

namespace agdraw::engine {

// Tracé libre créé par l'outil plume : segments droits pour l'instant ;
// les courbes de Bézier et l'édition de nœuds viendront à l'Étape 3.
class PathShape : public Shape {
public:
    explicit PathShape(QVector<QPointF> points) : points(std::move(points)) {}

    QRectF bounds() const override;
    bool contains(const QPointF &point) const override;
    void translate(const QPointF &delta) override;
    void paint(QPainter &painter) const override;

    QVector<QPointF> points;

private:
    QPainterPath toPath() const;
};

} // namespace agdraw::engine
