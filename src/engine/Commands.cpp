#include "Commands.h"

#include "Layer.h"

namespace agdraw::engine {

AddShapeCommand::AddShapeCommand(Layer *layer, std::unique_ptr<Shape> shape, const QString &text)
    : QUndoCommand(text), m_layer(layer), m_shape(std::move(shape)), m_shapePtr(m_shape.get()) {}

void AddShapeCommand::redo() {
    if (m_shape) {
        m_layer->addShape(std::move(m_shape));
    }
}

void AddShapeCommand::undo() {
    m_shape = m_layer->takeShape(m_shapePtr);
}

TranslateShapeCommand::TranslateShapeCommand(Shape *shape, const QPointF &delta)
    : QUndoCommand(QObject::tr("Déplacer")), m_shape(shape), m_delta(delta) {}

void TranslateShapeCommand::redo() {
    m_shape->translate(m_delta);
}

void TranslateShapeCommand::undo() {
    m_shape->translate(-m_delta);
}

ResizeShapeCommand::ResizeShapeCommand(Shape *shape, const QRectF &oldBounds, const QRectF &newBounds)
    : QUndoCommand(QObject::tr("Redimensionner")), m_shape(shape), m_oldBounds(oldBounds), m_newBounds(newBounds) {}

void ResizeShapeCommand::redo() {
    m_shape->setBounds(m_newBounds);
}

void ResizeShapeCommand::undo() {
    m_shape->setBounds(m_oldBounds);
}

} // namespace agdraw::engine
