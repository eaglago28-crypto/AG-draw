#pragma once

#include "Shape.h"

#include <QPainterPath>
#include <QPointF>
#include <QVector>
#include <vector>

namespace agdraw::engine {

// Chemin composé : plusieurs sous-tracés fermés combinés en une seule
// forme, remplis avec la règle pair-impair de QPainterPath (par défaut).
// Une lettre avec un trou (« O », « A », « e »…) a besoin de deux
// sous-tracés — le contour extérieur et celui du trou — pour se dessiner
// et se découper correctement ; c'est ce que produit la conversion de
// texte en courbes (CanvasView::convertSelectionToCurves).
class CompoundPathShape : public Shape {
public:
    explicit CompoundPathShape(std::vector<QVector<QPointF>> contours);

    QRectF bounds() const override;
    bool contains(const QPointF &point) const override;
    void translate(const QPointF &delta) override;
    void setBounds(const QRectF &rect) override;
    bool isResizable() const override { return true; }
    void paint(QPainter &painter) const override;

    const std::vector<QVector<QPointF>> &contours() const { return m_contours; }

private:
    QPainterPath toPath() const;

    std::vector<QVector<QPointF>> m_contours;
};

} // namespace agdraw::engine
