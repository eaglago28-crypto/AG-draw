#include "Commands.h"

#include "Document.h"
#include "Layer.h"
#include "Page.h"
#include "PowerClipGroup.h"

#include <algorithm>

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

RemoveShapeCommand::RemoveShapeCommand(Layer *layer, Shape *shape, const QString &text)
    : QUndoCommand(text), m_layer(layer), m_shapePtr(shape) {}

void RemoveShapeCommand::redo() {
    m_index = m_layer->indexOf(m_shapePtr);
    m_shape = m_layer->takeShape(m_shapePtr);
}

void RemoveShapeCommand::undo() {
    m_shapePtr = m_layer->insertShape(m_index, std::move(m_shape));
}

SetFillColorCommand::SetFillColorCommand(Shape *shape, const QColor &oldColor, const QColor &newColor)
    : QUndoCommand(QObject::tr("Couleur")), m_shape(shape), m_oldColor(oldColor), m_newColor(newColor) {}

void SetFillColorCommand::redo() {
    m_shape->fillColor = m_newColor;
}

void SetFillColorCommand::undo() {
    m_shape->fillColor = m_oldColor;
}

SetStrokeColorCommand::SetStrokeColorCommand(Shape *shape, const QColor &oldColor, const QColor &newColor)
    : QUndoCommand(QObject::tr("Couleur")), m_shape(shape), m_oldColor(oldColor), m_newColor(newColor) {}

void SetStrokeColorCommand::redo() {
    m_shape->strokeColor = m_newColor;
}

void SetStrokeColorCommand::undo() {
    m_shape->strokeColor = m_oldColor;
}

SetStrokeWidthCommand::SetStrokeWidthCommand(Shape *shape, qreal oldWidth, qreal newWidth)
    : QUndoCommand(QObject::tr("Épaisseur de trait")), m_shape(shape), m_oldWidth(oldWidth), m_newWidth(newWidth) {}

void SetStrokeWidthCommand::redo() {
    m_shape->strokeWidth = m_newWidth;
}

void SetStrokeWidthCommand::undo() {
    m_shape->strokeWidth = m_oldWidth;
}

ReorderShapeCommand::ReorderShapeCommand(Layer *layer, size_t fromIndex, size_t toIndex, const QString &text)
    : QUndoCommand(text), m_layer(layer), m_from(fromIndex), m_to(toIndex) {}

void ReorderShapeCommand::redo() {
    m_layer->moveShape(m_from, m_to);
}

void ReorderShapeCommand::undo() {
    m_layer->moveShape(m_to, m_from);
}

SetShadowCommand::SetShadowCommand(Shape *shape, bool oldEnabled, bool newEnabled)
    : QUndoCommand(QObject::tr("Ombre portée")), m_shape(shape), m_oldEnabled(oldEnabled), m_newEnabled(newEnabled) {}

void SetShadowCommand::redo() {
    m_shape->shadowEnabled = m_newEnabled;
}

void SetShadowCommand::undo() {
    m_shape->shadowEnabled = m_oldEnabled;
}

SetGradientCommand::SetGradientCommand(Shape *shape, bool oldEnabled, bool newEnabled)
    : QUndoCommand(QObject::tr("Dégradé")), m_shape(shape), m_oldEnabled(oldEnabled), m_newEnabled(newEnabled) {}

void SetGradientCommand::redo() {
    m_shape->gradientEnabled = m_newEnabled;
}

void SetGradientCommand::undo() {
    m_shape->gradientEnabled = m_oldEnabled;
}

SetContourCommand::SetContourCommand(Shape *shape, bool oldEnabled, bool newEnabled)
    : QUndoCommand(QObject::tr("Contour")), m_shape(shape), m_oldEnabled(oldEnabled), m_newEnabled(newEnabled) {}

void SetContourCommand::redo() {
    m_shape->contourEnabled = m_newEnabled;
}

void SetContourCommand::undo() {
    m_shape->contourEnabled = m_oldEnabled;
}

SetEnvelopeCommand::SetEnvelopeCommand(Shape *shape, bool oldEnabled, bool newEnabled, QVector<QPointF> oldCorners,
                                        QVector<QPointF> newCorners)
    : QUndoCommand(QObject::tr("Enveloppe")), m_shape(shape), m_oldEnabled(oldEnabled), m_newEnabled(newEnabled),
      m_oldCorners(std::move(oldCorners)), m_newCorners(std::move(newCorners)) {}

void SetEnvelopeCommand::redo() {
    m_shape->envelopeEnabled = m_newEnabled;
    m_shape->envelopeCorners = m_newCorners;
}

void SetEnvelopeCommand::undo() {
    m_shape->envelopeEnabled = m_oldEnabled;
    m_shape->envelopeCorners = m_oldCorners;
}

SetEnvelopeCornersCommand::SetEnvelopeCornersCommand(Shape *shape, QVector<QPointF> oldCorners,
                                                       QVector<QPointF> newCorners)
    : QUndoCommand(QObject::tr("Déformer l'enveloppe")), m_shape(shape), m_oldCorners(std::move(oldCorners)),
      m_newCorners(std::move(newCorners)) {}

void SetEnvelopeCornersCommand::redo() {
    m_shape->envelopeCorners = m_newCorners;
}

void SetEnvelopeCornersCommand::undo() {
    m_shape->envelopeCorners = m_oldCorners;
}

SetExtrusionCommand::SetExtrusionCommand(Shape *shape, bool oldEnabled, bool newEnabled)
    : QUndoCommand(QObject::tr("Extrusion")), m_shape(shape), m_oldEnabled(oldEnabled), m_newEnabled(newEnabled) {}

void SetExtrusionCommand::redo() {
    m_shape->extrusionEnabled = m_newEnabled;
}

void SetExtrusionCommand::undo() {
    m_shape->extrusionEnabled = m_oldEnabled;
}

ApplyPowerClipCommand::ApplyPowerClipCommand(Layer *layer, QVector<Shape *> members, Shape *container,
                                              const QString &text)
    : QUndoCommand(text), m_layer(layer), m_members(std::move(members)), m_container(container) {}

Shape *ApplyPowerClipCommand::groupPtr() const {
    return m_groupPtr;
}

void ApplyPowerClipCommand::redo() {
    m_originalIndices.clear();
    for (Shape *member : m_members) {
        m_originalIndices.append(m_layer->indexOf(member));
    }
    const size_t containerOriginalIndex = m_layer->indexOf(m_container);

    std::unique_ptr<Shape> container;
    std::vector<std::unique_ptr<Shape>> contents;
    for (Shape *member : m_members) {
        std::unique_ptr<Shape> owned = m_layer->takeShape(member);
        if (member == m_container) {
            container = std::move(owned);
        } else {
            contents.push_back(std::move(owned));
        }
    }

    size_t insertionIndex = containerOriginalIndex;
    for (int i = 0; i < m_members.size(); ++i) {
        if (m_members[i] != m_container && m_originalIndices[i] < containerOriginalIndex) {
            --insertionIndex;
        }
    }

    if (!m_detachedGroup) {
        m_detachedGroup = std::make_unique<PowerClipGroup>(std::move(container), std::move(contents));
        m_groupPtr = m_detachedGroup.get();
    } else {
        m_groupPtr->setContainer(std::move(container));
        m_groupPtr->setContents(std::move(contents));
    }
    m_layer->insertShape(insertionIndex, std::move(m_detachedGroup));
}

void ApplyPowerClipCommand::undo() {
    std::unique_ptr<Shape> takenAsShape = m_layer->takeShape(m_groupPtr);
    m_detachedGroup.reset(static_cast<PowerClipGroup *>(takenAsShape.release()));

    std::unique_ptr<Shape> container = m_detachedGroup->releaseContainer();
    std::vector<std::unique_ptr<Shape>> contents = m_detachedGroup->releaseContents();

    struct Restored {
        size_t originalIndex;
        std::unique_ptr<Shape> shape;
    };
    std::vector<Restored> restored;
    size_t contentIndex = 0;
    for (int i = 0; i < m_members.size(); ++i) {
        if (m_members[i] == m_container) {
            restored.push_back({m_originalIndices[i], std::move(container)});
        } else {
            restored.push_back({m_originalIndices[i], std::move(contents[contentIndex++])});
        }
    }
    std::sort(restored.begin(), restored.end(),
              [](const Restored &a, const Restored &b) { return a.originalIndex < b.originalIndex; });
    for (Restored &item : restored) {
        m_layer->insertShape(item.originalIndex, std::move(item.shape));
    }
}

AddPageCommand::AddPageCommand(Document *document, std::unique_ptr<Page> page, const QString &text)
    : QUndoCommand(text), m_document(document), m_page(std::move(page)), m_pagePtr(m_page.get()) {}

void AddPageCommand::redo() {
    if (m_page) {
        m_document->addPage(std::move(m_page));
    } else {
        m_document->setActivePage(m_pagePtr);
    }
}

void AddPageCommand::undo() {
    m_page = m_document->takePage(m_pagePtr);
}

RemovePageCommand::RemovePageCommand(Document *document, Page *page, const QString &text)
    : QUndoCommand(text), m_document(document), m_pagePtr(page) {}

void RemovePageCommand::redo() {
    m_index = m_document->indexOfPage(m_pagePtr);
    m_page = m_document->takePage(m_pagePtr);
}

void RemovePageCommand::undo() {
    m_pagePtr = m_document->insertPage(m_index, std::move(m_page));
}

} // namespace agdraw::engine
