#pragma once

#include "Shape.h"

#include <QString>

namespace agdraw::engine {

// Bloc de texte : position = point de départ (ligne de base de la première
// ligne) ; les sauts de ligne ('\n') sont pris en charge. La mise en page
// avancée (styles de caractères/paragraphes, texte sur chemin) arrivera
// avec le module Texte dédié.
class TextShape : public Shape {
public:
    TextShape(QPointF position, QString text) : position(position), text(std::move(text)) {
        // Le texte doit rester lisible par défaut, contrairement au gris pâle
        // hérité de Shape qui sert de remplissage neutre pour les formes.
        fillColor = Qt::black;
    }

    QRectF bounds() const override;
    bool contains(const QPointF &point) const override;
    void translate(const QPointF &delta) override { position += delta; }
    void paint(QPainter &painter) const override;

    QPointF position;
    QString text;
    qreal fontPointSize = 24.0;
};

} // namespace agdraw::engine
