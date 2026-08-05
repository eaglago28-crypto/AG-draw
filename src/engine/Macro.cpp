#include "Macro.h"

#include "Commands.h"
#include "Document.h"

#include <QUndoStack>

namespace agdraw::engine {

void applyMacro(Document &document, const Macro &macro, const QVector<Shape *> &targets) {
    if (targets.isEmpty() || macro.steps.isEmpty()) {
        return;
    }

    QUndoStack *undoStack = document.undoStack();
    undoStack->beginMacro(QObject::tr("Macro : %1").arg(macro.name));
    for (const MacroStep &step : macro.steps) {
        for (Shape *shape : targets) {
            switch (step.kind) {
                case MacroStep::Kind::SetFillColor:
                    undoStack->push(new SetFillColorCommand(shape, shape->fillColor, step.color));
                    break;
                case MacroStep::Kind::SetStrokeColor:
                    undoStack->push(new SetStrokeColorCommand(shape, shape->strokeColor, step.color));
                    break;
                case MacroStep::Kind::SetStrokeWidth:
                    undoStack->push(new SetStrokeWidthCommand(shape, shape->strokeWidth, step.value));
                    break;
                case MacroStep::Kind::SetShadow:
                    if (shape->supportsFillEffects()) {
                        undoStack->push(new SetShadowCommand(shape, shape->shadowEnabled, step.enabled));
                    }
                    break;
                case MacroStep::Kind::SetGradient:
                    if (shape->supportsFillEffects()) {
                        undoStack->push(new SetGradientCommand(shape, shape->gradientEnabled, step.enabled));
                    }
                    break;
                case MacroStep::Kind::SetContour:
                    if (shape->supportsFillEffects()) {
                        undoStack->push(new SetContourCommand(shape, shape->contourEnabled, step.enabled));
                    }
                    break;
                case MacroStep::Kind::SetExtrusion:
                    if (shape->supportsFillEffects()) {
                        undoStack->push(new SetExtrusionCommand(shape, shape->extrusionEnabled, step.enabled));
                    }
                    break;
                case MacroStep::Kind::Translate:
                    undoStack->push(new TranslateShapeCommand(shape, step.delta));
                    break;
            }
        }
    }
    undoStack->endMacro();
}

} // namespace agdraw::engine
