#pragma once

#include "Shape.h"

#include <QPainterPath>
#include <QVector>

namespace agdraw::engine {

// Point échantillonné le long d'un trait de pinceau : position et pression
// du stylet (0 à 1 ; 1.0 par défaut pour une souris sans tablette).
struct BrushPoint {
    QPointF point;
    qreal pressure = 1.0;
};

// Construit le contour (ruban) d'un trait à largeur variable : plus la
// pression est forte à un point donné, plus le trait est large à cet
// endroit. Utilisé aussi bien pour le rendu final (BrushStroke::paint)
// que pour l'aperçu en direct pendant le tracé.
QPainterPath buildBrushOutline(const QVector<BrushPoint> &points, qreal baseWidth);

// Trait de pinceau à largeur variable, dessiné en un seul geste continu
// (souris ou stylet). Contrairement au tracé de la plume (nœuds de
// Bézier édités un par un), c'est la forme naturelle pour une tablette
// graphique : la pression du stylet module l'épaisseur en temps réel.
class BrushStroke : public Shape {
public:
    explicit BrushStroke(QVector<BrushPoint> points, qreal baseWidth = 8.0)
        : points(std::move(points)), baseWidth(baseWidth) {}

    QRectF bounds() const override { return buildBrushOutline(points, baseWidth).boundingRect(); }
    bool contains(const QPointF &point) const override { return buildBrushOutline(points, baseWidth).contains(point); }
    void translate(const QPointF &delta) override;
    void paint(QPainter &painter) const override;

    QVector<BrushPoint> points;
    qreal baseWidth;
};

} // namespace agdraw::engine
