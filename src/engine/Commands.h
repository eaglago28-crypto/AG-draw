#pragma once

#include "Shape.h"

#include <QColor>
#include <QUndoCommand>
#include <QVector>
#include <memory>

namespace agdraw::engine {

class Layer;
class Document;
class Page;
class PowerClipGroup;

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

// Retire une forme existante d'un calque (annulable) en conservant sa
// position d'origine dans la pile de dessin pour une restauration fidèle.
class RemoveShapeCommand : public QUndoCommand {
public:
    RemoveShapeCommand(Layer *layer, Shape *shape, const QString &text);

    void redo() override;
    void undo() override;

private:
    Layer *m_layer;
    Shape *m_shapePtr;
    std::unique_ptr<Shape> m_shape;
    size_t m_index = 0;
};

// Change la couleur de remplissage d'une forme.
class SetFillColorCommand : public QUndoCommand {
public:
    SetFillColorCommand(Shape *shape, const QColor &oldColor, const QColor &newColor);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    QColor m_oldColor;
    QColor m_newColor;
};

// Change la couleur de trait d'une forme (utilisé pour les tracés, qui
// n'ont pas de remplissage).
class SetStrokeColorCommand : public QUndoCommand {
public:
    SetStrokeColorCommand(Shape *shape, const QColor &oldColor, const QColor &newColor);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    QColor m_oldColor;
    QColor m_newColor;
};

// Change l'épaisseur de trait d'une forme.
class SetStrokeWidthCommand : public QUndoCommand {
public:
    SetStrokeWidthCommand(Shape *shape, qreal oldWidth, qreal newWidth);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    qreal m_oldWidth;
    qreal m_newWidth;
};

// Change la position d'une forme dans l'ordre de dessin de son calque
// (avant-plan / arrière-plan).
class ReorderShapeCommand : public QUndoCommand {
public:
    ReorderShapeCommand(Layer *layer, size_t fromIndex, size_t toIndex, const QString &text);

    void redo() override;
    void undo() override;

private:
    Layer *m_layer;
    size_t m_from;
    size_t m_to;
};

// Active/désactive l'ombre portée d'une forme.
class SetShadowCommand : public QUndoCommand {
public:
    SetShadowCommand(Shape *shape, bool oldEnabled, bool newEnabled);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    bool m_oldEnabled;
    bool m_newEnabled;
};

// Active/désactive le remplissage en dégradé d'une forme.
class SetGradientCommand : public QUndoCommand {
public:
    SetGradientCommand(Shape *shape, bool oldEnabled, bool newEnabled);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    bool m_oldEnabled;
    bool m_newEnabled;
};

// Active/désactive l'effet Contour (copies concentriques) d'une forme.
class SetContourCommand : public QUndoCommand {
public:
    SetContourCommand(Shape *shape, bool oldEnabled, bool newEnabled);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    bool m_oldEnabled;
    bool m_newEnabled;
};

// Active/désactive l'effet Enveloppe (déformation par coins) d'une forme.
class SetEnvelopeCommand : public QUndoCommand {
public:
    SetEnvelopeCommand(Shape *shape, bool oldEnabled, bool newEnabled, QVector<QPointF> oldCorners,
                        QVector<QPointF> newCorners);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    bool m_oldEnabled;
    bool m_newEnabled;
    QVector<QPointF> m_oldCorners;
    QVector<QPointF> m_newCorners;
};

// Déplace un ou plusieurs coins de l'enveloppe d'une forme entre deux
// jeux de positions absolues (glisser une poignée d'enveloppe).
class SetEnvelopeCornersCommand : public QUndoCommand {
public:
    SetEnvelopeCornersCommand(Shape *shape, QVector<QPointF> oldCorners, QVector<QPointF> newCorners);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    QVector<QPointF> m_oldCorners;
    QVector<QPointF> m_newCorners;
};

// Active/désactive l'effet Extrusion (relief 3D simulé) d'une forme.
class SetExtrusionCommand : public QUndoCommand {
public:
    SetExtrusionCommand(Shape *shape, bool oldEnabled, bool newEnabled);

    void redo() override;
    void undo() override;

private:
    Shape *m_shape;
    bool m_oldEnabled;
    bool m_newEnabled;
};

// Regroupe plusieurs formes déjà présentes dans un calque en un PowerClip
// (dernière forme de `members` = contenant). redo() les retire du calque et
// les enveloppe dans un PowerClipGroup inséré à la position d'origine du
// contenant ; undo() démonte le groupe et restitue chaque forme à son index
// d'origine dans le calque, dans l'ordre. Le même objet PowerClipGroup est
// réutilisé (vidé/repeuplé) d'un cycle undo/redo à l'autre plutôt que
// recréé, pour que groupPtr() reste valide même après un undo suivi d'un
// redo (par ex. la sélection courante de l'UI qui le référence).
class ApplyPowerClipCommand : public QUndoCommand {
public:
    ApplyPowerClipCommand(Layer *layer, QVector<Shape *> members, Shape *container, const QString &text);

    void redo() override;
    void undo() override;

    Shape *groupPtr() const;

private:
    Layer *m_layer;
    QVector<Shape *> m_members;
    Shape *m_container;
    QVector<size_t> m_originalIndices;
    std::unique_ptr<PowerClipGroup> m_detachedGroup;
    PowerClipGroup *m_groupPtr = nullptr;
};

// Ajoute une page déjà construite au document (annulable).
class AddPageCommand : public QUndoCommand {
public:
    AddPageCommand(Document *document, std::unique_ptr<Page> page, const QString &text);

    void redo() override;
    void undo() override;

    Page *pagePtr() const { return m_pagePtr; }

private:
    Document *m_document;
    std::unique_ptr<Page> m_page;
    Page *m_pagePtr;
};

// Retire une page existante du document (annulable) en conservant sa
// position d'origine dans la liste pour une restauration fidèle.
class RemovePageCommand : public QUndoCommand {
public:
    RemovePageCommand(Document *document, Page *page, const QString &text);

    void redo() override;
    void undo() override;

private:
    Document *m_document;
    Page *m_pagePtr;
    std::unique_ptr<Page> m_page;
    size_t m_index = 0;
};

} // namespace agdraw::engine
