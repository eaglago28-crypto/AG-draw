#pragma once

#include "Shape.h"

#include <QPainterPath>
#include <QVector>

namespace agdraw::engine {

// Nœud d'un tracé : point d'ancrage + poignée sortante (décalage relatif).
// La poignée entrante est le symétrique (-handle), comme dans un outil
// plume classique. Une poignée nulle donne un point anguleux (segment
// droit).
struct PathNode {
    QPointF point;
    QPointF handle;
};

// Tracé libre créé par l'outil plume : chaque nœud peut être un point
// anguleux (segment droit) ou un point lisse avec poignées de Bézier.
class PathShape : public Shape {
public:
    explicit PathShape(QVector<PathNode> nodes) : nodes(std::move(nodes)) {}

    QRectF bounds() const override;
    bool contains(const QPointF &point) const override;
    void translate(const QPointF &delta) override;
    void paint(QPainter &painter) const override;

    QVector<PathNode> nodes;

private:
    QPainterPath toPath() const;
};

} // namespace agdraw::engine
