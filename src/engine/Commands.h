#pragma once

#include "Shape.h"

#include <QUndoCommand>
#include <memory>

namespace agdraw::engine {

class Layer;

// Ajoute une forme déjà construite à un calque (annulable). L'appelant
// perd la propriété de la forme au profit de la commande / du calque.
class AddShapeCommand : public QUndoCommand {
public:
    AddShapeCommand(Layer *layer, std::unique_ptr<Shape> shape, const QString &text);

    void redo() override;
    void undo() override;

    Shape *shapePtr() const { return m_shapePtr; }

private:
    Layer *m_layer;
    std::unique_ptr<Shape> m_shape;
    Shape *m_shapePtr;
};

// Déplacement d'une forme existante par un vecteur delta.
class TranslateShapeCommand : public QUndoCommand {
public:
    TranslateShapeCommand(Shape *shape, const QPointF &delta);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    QPointF m_delta;
};

// Redimensionnement d'une forme (rectangle, ellipse) entre deux rectangles
// englobants absolus.
class ResizeShapeCommand : public QUndoCommand {
public:
    ResizeShapeCommand(Shape *shape, const QRectF &oldBounds, const QRectF &newBounds);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    QRectF m_oldBounds;
    QRectF m_newBounds;
};

} // namespace agdraw::engine
