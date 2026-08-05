#pragma once

#include <QColor>
#include <QPointF>
#include <QString>
#include <QVector>

namespace agdraw::engine {

// Une étape élémentaire enregistrable/rejouable d'une macro. Chaque type
// n'utilise que le sous-ensemble de champs pertinent (voir applyMacroStep).
struct MacroStep {
    enum class Kind { SetFillColor, SetStrokeColor, SetStrokeWidth, SetShadow, SetGradient, SetContour, SetExtrusion, Translate };

    Kind kind;
    QColor color;
    qreal value = 0.0;
    bool enabled = false;
    QPointF delta;
};

// Séquence nommée d'étapes, enregistrée depuis les actions réelles de
// l'utilisateur sur le canevas (CanvasView::startMacroRecording) et
// rejouable sur n'importe quelle sélection (CanvasView::playMacro).
// Persistée au niveau du document (voir AgdDocumentIO), indépendamment des
// calques/pages : une macro n'est pas liée à des formes précises.
struct Macro {
    QString name;
    QVector<MacroStep> steps;
};

class Document;
class Shape;

// Rejoue toutes les étapes de `macro`, dans l'ordre, sur chaque forme de
// `targets` (une forme à la fois par étape, comme les bascules d'effet
// existantes sur une sélection multiple), regroupées en une seule opération
// annulable. Les étapes qui ne s'appliquent pas à une forme donnée (effets
// sur une forme sans supportsFillEffects()) sont silencieusement ignorées
// pour cette forme.
void applyMacro(Document &document, const Macro &macro, const QVector<Shape *> &targets);

} // namespace agdraw::engine
