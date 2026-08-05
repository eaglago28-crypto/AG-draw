#pragma once

#include <QColor>
#include <QPointF>
#include <QRectF>

class QPainter;

namespace agdraw::engine {

// Forme géométrique de base du document. Chaque outil de dessin produit
// une sous-classe concrète (rectangle, ellipse, tracé).
class Shape {
public:
    virtual ~Shape() = default;

    virtual QRectF bounds() const = 0;
    virtual bool contains(const QPointF &point) const = 0;
    virtual void translate(const QPointF &delta) = 0;
    virtual void paint(QPainter &painter) const = 0;

    // Redimensionnement par glisser (rectangle, ellipse). Sans effet pour
    // les formes construites point par point (le tracé de la plume).
    virtual void setBounds(const QRectF &rect) { (void)rect; }

    QColor fillColor = QColor(200, 205, 215);
    QColor strokeColor = Qt::black;
    qreal strokeWidth = 1.0;
};

} // namespace agdraw::engine
