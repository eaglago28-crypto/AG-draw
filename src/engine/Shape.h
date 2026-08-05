#pragma once

#include <QColor>
#include <QLinearGradient>
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
    virtual bool isResizable() const { return false; }

    // Ombre portée et dégradé de remplissage : pris en charge par les
    // formes à surface (Rectangle, Ellipse) uniquement pour l'instant.
    virtual bool supportsFillEffects() const { return false; }

    QColor fillColor = QColor(200, 205, 215);
    QColor strokeColor = Qt::black;
    qreal strokeWidth = 1.0;

    bool shadowEnabled = false;
    QColor shadowColor = QColor(0, 0, 0, 120);
    QPointF shadowOffset = QPointF(6, 6);

    bool gradientEnabled = false;
    QColor gradientStartColor = Qt::white;
    QColor gradientEndColor = Qt::gray;
    qreal gradientAngle = 0.0; // degrés, 0 = gauche → droite

    // Contour (CorelDRAW) : copies concentriques du contour, dégradées de
    // la couleur de remplissage vers contourColor. contourOffset positif
    // fait grandir les copies vers l'extérieur, négatif vers l'intérieur.
    bool contourEnabled = false;
    int contourSteps = 4;
    qreal contourOffset = 8.0;
    QColor contourColor = Qt::black;
};

// Construit un dégradé linéaire couvrant `bounds`, orienté selon
// `angleDegrees`. Partagé par les formes qui prennent en charge le
// remplissage en dégradé.
QLinearGradient makeShapeGradient(const QRectF &bounds, const QColor &start, const QColor &end, qreal angleDegrees);

// Interpole linéairement entre deux couleurs (t=0 → a, t=1 → b), y compris
// le canal alpha. Utilisé par l'effet Contour pour dégrader les copies
// concentriques vers contourColor.
QColor interpolateColor(const QColor &a, const QColor &b, qreal t);

} // namespace agdraw::engine
