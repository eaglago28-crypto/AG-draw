#pragma once

#include "Shape.h"

#include <memory>
#include <vector>

namespace agdraw::engine {

class TextShape;

// Convertit un TextShape en formes vectorielles éditables (« Convertir en
// courbes ») : une CompoundPathShape par caractère non-blanc, positionnée
// selon la même police/taille/interligne que TextShape::paint(). Chaque
// lettre à trou (« O », « A », « e »…) obtient ses sous-tracés extérieur
// et intérieur dans la même forme, correctement évidée.
//
// Limitation connue : l'avance de chaque caractère utilise
// QFontMetricsF::horizontalAdvance() lettre par lettre, sans les paires de
// crénage (kerning) que rendrait un layout de police complet — l'espacement
// peut donc différer très légèrement de TextShape::paint() sur certaines
// polices/paires de lettres.
std::vector<std::unique_ptr<Shape>> textToCurves(const TextShape &text);

} // namespace agdraw::engine
