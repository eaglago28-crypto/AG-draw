#pragma once

#include "Shape.h"

#include <QPointF>
#include <QVector>
#include <memory>

namespace agdraw::engine {

enum class RecognizedShapeKind { None, Rectangle, Ellipse };

struct ShapeRecognitionResult {
    RecognizedShapeKind kind = RecognizedShapeKind::None;
    std::unique_ptr<Shape> shape; // nullptr si kind == None
};

// Reconnaissance de formes géométriques (« croquis-vers-vecteur » /
// correction automatique de formes) : tente d'ajuster un tracé libre
// échantillonné (par ex. les points d'un BrushStroke) à un rectangle ou
// une ellipse propre. N'est reconnu que si le tracé forme une boucle à peu
// près fermée (début et fin proches l'un de l'autre par rapport à sa
// taille) ET colle d'assez près au périmètre de la forme candidate (erreur
// quadratique moyenne normalisée sous un seuil) — un simple trait ouvert
// ou un gribouillis quelconque n'est jamais reconnu. Approche géométrique
// déterministe, sans modèle d'apprentissage.
ShapeRecognitionResult recognizeShape(const QVector<QPointF> &points);

} // namespace agdraw::engine
