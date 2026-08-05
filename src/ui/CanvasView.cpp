#include "CanvasView.h"

#include "Commands.h"
#include "Document.h"
#include "DocumentItem.h"
#include "EllipseShape.h"
#include "Layer.h"
#include "RectShape.h"
#include "TextShape.h"

#include <QGraphicsScene>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QUndoStack>
#include <QWheelEvent>

namespace agdraw::ui {

namespace {
constexpr qreal kSceneExtent = 100000.0;
constexpr qreal kGridStep = 50.0;
constexpr qreal kMinZoom = 0.05;
constexpr qreal kMaxZoom = 40.0;
constexpr qreal kHandleRadiusPx = 6.0;
constexpr qreal kMinShapeSize = 2.0;
constexpr qreal kMinPenHandleLength = 3.0;
} // namespace

CanvasView::CanvasView(QWidget *parent)
    : QGraphicsView(parent), m_document(std::make_unique<engine::Document>()) {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setupScene();
    setupTextEditor();
}

CanvasView::~CanvasView() = default;

engine::Document &CanvasView::document() {
    return *m_document;
}

void CanvasView::setupScene() {
    auto *scene = new QGraphicsScene(-kSceneExtent, -kSceneExtent, 2 * kSceneExtent, 2 * kSceneExtent, this);
    setScene(scene);

    // Page de travail par défaut (A4 à 96 DPI), pour donner un repère visuel
    // au centre de la zone infinie.
    auto *page = scene->addRect(0, 0, 794, 1123, QPen(Qt::NoPen), QBrush(Qt::white));
    page->setZValue(0);

    m_documentItem = new DocumentItem(*m_document, QRectF(-kSceneExtent, -kSceneExtent, 2 * kSceneExtent, 2 * kSceneExtent));
    m_documentItem->setZValue(1);
    scene->addItem(m_documentItem);

    centerOn(794 / 2.0, 1123 / 2.0);
}

void CanvasView::setupTextEditor() {
    m_textEditor = new QLineEdit(viewport());
    m_textEditor->hide();
    m_textEditor->installEventFilter(this);
    connect(m_textEditor, &QLineEdit::returnPressed, this, &CanvasView::commitTextEditor);
}

void CanvasView::setActiveTool(Tool tool) {
    m_penNodes.clear();
    m_penDraggingHandle = false;
    m_rubberBanding = false;
    m_resizing = false;
    m_dragging = false;
    if (m_textEditor->isVisible()) {
        cancelTextEditor();
    }
    m_activeTool = tool;
    m_selection.clear();
    m_documentItem->update();
}

void CanvasView::setActiveColor(const QColor &color) {
    m_currentColor = color;
    m_colorExplicitlySet = true;
    if (!m_selection.isEmpty()) {
        m_document->undoStack()->beginMacro(tr("Couleur"));
        for (engine::Shape *shape : m_selection) {
            if (dynamic_cast<engine::PathShape *>(shape)) {
                m_document->undoStack()->push(new engine::SetStrokeColorCommand(shape, shape->strokeColor, color));
            } else {
                m_document->undoStack()->push(new engine::SetFillColorCommand(shape, shape->fillColor, color));
            }
        }
        m_document->undoStack()->endMacro();
        m_documentItem->update();
    }
    emit statusMessage(tr("Couleur active : %1").arg(color.name()));
}

void CanvasView::applyCurrentColor(engine::Shape *shape) const {
    if (dynamic_cast<engine::PathShape *>(shape)) {
        shape->strokeColor = m_currentColor;
    } else {
        shape->fillColor = m_currentColor;
    }
}

void CanvasView::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
        const qreal newZoom = m_zoom * factor;
        if (newZoom >= kMinZoom && newZoom <= kMaxZoom) {
            m_zoom = newZoom;
            scale(factor, factor);
        }
        event->accept();
        return;
    }
    QGraphicsView::wheelEvent(event);
}

void CanvasView::drawBackground(QPainter *painter, const QRectF &rect) {
    painter->fillRect(rect, QColor(60, 60, 64));

    QPen gridPen(QColor(72, 72, 78));
    gridPen.setWidth(0);
    painter->setPen(gridPen);

    const qreal left = std::floor(rect.left() / kGridStep) * kGridStep;
    const qreal top = std::floor(rect.top() / kGridStep) * kGridStep;

    for (qreal x = left; x < rect.right(); x += kGridStep) {
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    for (qreal y = top; y < rect.bottom(); y += kGridStep) {
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
    }
}

void CanvasView::drawForeground(QPainter *painter, const QRectF &) {
    const QColor accent(0, 122, 255);

    if (m_creatingShape) {
        QPen previewPen(accent);
        previewPen.setStyle(Qt::DashLine);
        previewPen.setWidth(0);
        painter->setPen(previewPen);
        painter->setBrush(QColor(accent.red(), accent.green(), accent.blue(), 40));
        if (m_activeTool == Tool::Rectangle) {
            painter->drawRect(QRectF(m_dragStart, m_dragCurrent).normalized());
        } else if (m_activeTool == Tool::Ellipse) {
            painter->drawEllipse(QRectF(m_dragStart, m_dragCurrent).normalized());
        }
    }

    if (m_activeTool == Tool::Selection) {
        for (engine::Shape *shape : m_selection) {
            const bool isPrimaryResizing = m_resizing && m_selection.size() == 1 && shape == m_selection.first();
            const QRectF bounds = isPrimaryResizing ? m_pendingBounds : shape->bounds();
            QPen handlePen(accent);
            handlePen.setStyle(Qt::DashLine);
            handlePen.setWidth(0);
            painter->setPen(handlePen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(bounds.adjusted(-2, -2, 2, 2));
        }

        if (m_selection.size() == 1 && m_selection.first()->isResizable()) {
            const QRectF bounds = m_resizing ? m_pendingBounds : m_selection.first()->bounds();
            const qreal handleSize = kHandleRadiusPx / std::max(m_zoom, 0.01);
            painter->setBrush(Qt::white);
            painter->setPen(QPen(accent, 0));
            for (const QPointF &corner : {bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()}) {
                painter->drawRect(QRectF(corner - QPointF(handleSize, handleSize) / 2, QSizeF(handleSize, handleSize)));
            }
        }

        if (m_rubberBanding) {
            QPen bandPen(accent);
            bandPen.setStyle(Qt::DashLine);
            bandPen.setWidth(0);
            painter->setPen(bandPen);
            painter->setBrush(QColor(accent.red(), accent.green(), accent.blue(), 30));
            painter->drawRect(m_rubberBandRect);
        }
    }

    if (!m_penNodes.isEmpty()) {
        QPen previewPen(accent);
        previewPen.setStyle(Qt::DashLine);
        previewPen.setWidth(0);
        painter->setPen(previewPen);
        painter->setBrush(Qt::NoBrush);

        QPainterPath preview;
        preview.moveTo(m_penNodes.first().point);
        for (int i = 1; i < m_penNodes.size(); ++i) {
            const engine::PathNode &prev = m_penNodes[i - 1];
            const engine::PathNode &cur = m_penNodes[i];
            if (prev.handle.isNull() && cur.handle.isNull()) {
                preview.lineTo(cur.point);
            } else {
                preview.cubicTo(prev.point + prev.handle, cur.point - cur.handle, cur.point);
            }
        }
        painter->drawPath(preview);

        painter->setBrush(accent);
        for (const engine::PathNode &node : m_penNodes) {
            painter->drawEllipse(node.point, 3, 3);
            if (!node.handle.isNull()) {
                painter->drawLine(node.point - node.handle, node.point + node.handle);
                painter->drawRect(QRectF(node.point + node.handle - QPointF(2, 2), QSizeF(4, 4)));
                painter->drawRect(QRectF(node.point - node.handle - QPointF(2, 2), QSizeF(4, 4)));
            }
        }
    }
}

QRectF CanvasView::computeResizedBounds(const QPointF &scenePos) const {
    QRectF newBounds = m_originalBounds;
    switch (m_activeHandle) {
        case 0: newBounds.setTopLeft(scenePos); break;
        case 1: newBounds.setTopRight(scenePos); break;
        case 2: newBounds.setBottomLeft(scenePos); break;
        case 3: newBounds.setBottomRight(scenePos); break;
        default: break;
    }
    return newBounds.normalized();
}

int CanvasView::hitTestHandle(const QPoint &viewPos) const {
    if (m_selection.size() != 1 || !m_selection.first()->isResizable()) {
        return -1;
    }
    const QRectF bounds = m_selection.first()->bounds();
    const QPointF corners[4] = {bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()};
    for (int i = 0; i < 4; ++i) {
        const QPoint handlePos = mapFromScene(corners[i]);
        if ((handlePos - viewPos).manhattanLength() <= kHandleRadiusPx * 2) {
            return i;
        }
    }
    return -1;
}

QVector<engine::Shape *> CanvasView::shapesInRect(const QRectF &rect) const {
    QVector<engine::Shape *> result;
    for (const auto &layer : m_document->layers()) {
        if (!layer->isVisible() || layer->isLocked()) {
            continue;
        }
        for (const auto &shape : layer->shapes()) {
            if (rect.intersects(shape->bounds())) {
                result.append(shape.get());
            }
        }
    }
    return result;
}

void CanvasView::mousePressEvent(QMouseEvent *event) {
    if (m_textEditor->isVisible()) {
        commitTextEditor();
    }

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    const QPointF scenePos = mapToScene(event->pos());

    switch (m_activeTool) {
        case Tool::Selection: {
            const int handle = hitTestHandle(event->pos());
            if (handle >= 0) {
                m_resizing = true;
                m_activeHandle = handle;
                m_originalBounds = m_selection.first()->bounds();
                m_pendingBounds = m_originalBounds;
                break;
            }

            engine::Shape *hitShape = m_document->shapeAt(scenePos);
            const bool shiftHeld = event->modifiers() & Qt::ShiftModifier;
            if (hitShape) {
                if (shiftHeld) {
                    if (m_selection.contains(hitShape)) {
                        m_selection.removeAll(hitShape);
                    } else {
                        m_selection.append(hitShape);
                    }
                } else if (!m_selection.contains(hitShape)) {
                    m_selection = {hitShape};
                }
                m_dragging = !m_selection.isEmpty();
                m_dragStart = scenePos;
                m_lastMovePos = scenePos;
                emit statusMessage(tr("%1 forme(s) sélectionnée(s)").arg(m_selection.size()));
            } else {
                if (!shiftHeld) {
                    m_selection.clear();
                }
                m_rubberBanding = true;
                m_rubberBandStart = scenePos;
                m_rubberBandRect = QRectF(scenePos, scenePos);
                emit statusMessage(tr("Aucune forme sous le curseur"));
            }
            break;
        }
        case Tool::Rectangle:
        case Tool::Ellipse:
            m_dragStart = scenePos;
            m_dragCurrent = scenePos;
            m_creatingShape = true;
            break;
        case Tool::Pen:
            m_penNodes.append(engine::PathNode{scenePos, QPointF(0, 0)});
            m_penDraggingHandle = true;
            emit statusMessage(tr("Plume : %1 point(s) — glisser pour une courbe, double-cliquez pour terminer, "
                                   "Échap pour annuler")
                                    .arg(m_penNodes.size()));
            break;
        case Tool::Text: {
            m_textEditorScenePos = scenePos;
            const QPoint viewPos = event->pos();
            m_textEditor->setGeometry(viewPos.x(), viewPos.y(), 220, 30);
            m_textEditor->clear();
            m_textEditor->show();
            m_textEditor->setFocus();
            break;
        }
    }

    m_documentItem->update();
    event->accept();
}

void CanvasView::mouseMoveEvent(QMouseEvent *event) {
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    const QPointF scenePos = mapToScene(event->pos());

    if (m_resizing) {
        m_pendingBounds = computeResizedBounds(scenePos);
        m_selection.first()->setBounds(m_pendingBounds);
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_creatingShape) {
        m_dragCurrent = scenePos;
        viewport()->update();
        event->accept();
        return;
    }

    if (m_rubberBanding) {
        m_rubberBandRect = QRectF(m_rubberBandStart, scenePos).normalized();
        viewport()->update();
        event->accept();
        return;
    }

    if (m_activeTool == Tool::Pen && m_penDraggingHandle && !m_penNodes.isEmpty()) {
        m_penNodes.last().handle = scenePos - m_penNodes.last().point;
        viewport()->update();
        event->accept();
        return;
    }

    if (!m_dragging) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    if (m_activeTool == Tool::Selection && !m_selection.isEmpty()) {
        const QPointF delta = scenePos - m_lastMovePos;
        for (engine::Shape *shape : m_selection) {
            shape->translate(delta);
        }
        m_lastMovePos = scenePos;
    }

    m_documentItem->update();
    event->accept();
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }

    if (m_resizing) {
        m_resizing = false;
        const QRectF finalBounds = computeResizedBounds(mapToScene(event->pos()));
        engine::Shape *shape = m_selection.first();
        shape->setBounds(m_originalBounds);
        if (finalBounds != m_originalBounds) {
            m_document->undoStack()->push(new engine::ResizeShapeCommand(shape, m_originalBounds, finalBounds));
        }
        emit statusMessage(tr("Prêt"));
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_creatingShape) {
        m_creatingShape = false;
        const QRectF rect = QRectF(m_dragStart, mapToScene(event->pos())).normalized();
        if (rect.width() > kMinShapeSize && rect.height() > kMinShapeSize) {
            std::unique_ptr<engine::Shape> shape;
            QString label;
            if (m_activeTool == Tool::Rectangle) {
                shape = std::make_unique<engine::RectShape>(rect);
                label = tr("Rectangle");
            } else {
                shape = std::make_unique<engine::EllipseShape>(rect);
                label = tr("Ellipse");
            }
            applyCurrentColor(shape.get());
            auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), label);
            m_document->undoStack()->push(command);
            m_selection = {command->shapePtr()};
        }
        m_documentItem->update();
        viewport()->update();
        event->accept();
        return;
    }

    if (m_rubberBanding) {
        m_rubberBanding = false;
        const QVector<engine::Shape *> found = shapesInRect(m_rubberBandRect);
        for (engine::Shape *shape : found) {
            if (!m_selection.contains(shape)) {
                m_selection.append(shape);
            }
        }
        m_rubberBandRect = QRectF();
        emit statusMessage(m_selection.isEmpty() ? tr("Aucune forme sous le curseur")
                                                   : tr("%1 forme(s) sélectionnée(s)").arg(m_selection.size()));
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_activeTool == Tool::Pen && m_penDraggingHandle) {
        m_penDraggingHandle = false;
        QPointF &handle = m_penNodes.last().handle;
        if (QPointF::dotProduct(handle, handle) < kMinPenHandleLength * kMinPenHandleLength) {
            handle = QPointF(0, 0);
        }
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_dragging) {
        m_dragging = false;
        if (m_activeTool == Tool::Selection && !m_selection.isEmpty()) {
            const QPointF totalDelta = mapToScene(event->pos()) - m_dragStart;
            if (!totalDelta.isNull()) {
                for (engine::Shape *shape : m_selection) {
                    shape->translate(-totalDelta);
                }
                if (m_selection.size() == 1) {
                    m_document->undoStack()->push(new engine::TranslateShapeCommand(m_selection.first(), totalDelta));
                } else {
                    m_document->undoStack()->beginMacro(tr("Déplacer la sélection"));
                    for (engine::Shape *shape : m_selection) {
                        m_document->undoStack()->push(new engine::TranslateShapeCommand(shape, totalDelta));
                    }
                    m_document->undoStack()->endMacro();
                }
            }
        }
        emit statusMessage(tr("Prêt"));
    }

    m_documentItem->update();
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_activeTool == Tool::Pen && m_penNodes.size() >= 2) {
        auto shape = std::make_unique<engine::PathShape>(m_penNodes);
        m_penNodes.clear();
        m_penDraggingHandle = false;
        applyCurrentColor(shape.get());
        auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), tr("Tracé"));
        m_document->undoStack()->push(command);
        m_documentItem->update();
        emit statusMessage(tr("Tracé terminé"));
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void CanvasView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && !m_penNodes.isEmpty()) {
        m_penNodes.clear();
        m_penDraggingHandle = false;
        m_documentItem->update();
        emit statusMessage(tr("Tracé annulé"));
        event->accept();
        return;
    }

    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && m_activeTool == Tool::Selection &&
        !m_selection.isEmpty()) {
        m_document->undoStack()->beginMacro(tr("Supprimer"));
        for (engine::Shape *shape : m_selection) {
            engine::Layer *owner = m_document->findLayerOf(shape);
            if (owner) {
                m_document->undoStack()->push(new engine::RemoveShapeCommand(owner, shape, tr("Supprimer")));
            }
        }
        m_document->undoStack()->endMacro();
        m_selection.clear();
        m_documentItem->update();
        emit statusMessage(tr("Forme(s) supprimée(s)"));
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Undo)) {
        m_document->undoStack()->undo();
        m_selection.clear();
        m_documentItem->update();
        emit statusMessage(tr("Annulé"));
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Redo)) {
        m_document->undoStack()->redo();
        m_selection.clear();
        m_documentItem->update();
        emit statusMessage(tr("Rétabli"));
        event->accept();
        return;
    }

    QGraphicsView::keyPressEvent(event);
}

bool CanvasView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_textEditor && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            cancelTextEditor();
            return true;
        }
    }
    return QGraphicsView::eventFilter(watched, event);
}

void CanvasView::commitTextEditor() {
    if (!m_textEditor->isVisible()) {
        return;
    }
    const QString text = m_textEditor->text().trimmed();
    m_textEditor->hide();
    if (!text.isEmpty()) {
        auto shape = std::make_unique<engine::TextShape>(m_textEditorScenePos, text);
        if (m_colorExplicitlySet) {
            shape->fillColor = m_currentColor;
        }
        auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), tr("Texte"));
        m_document->undoStack()->push(command);
        m_documentItem->update();
    }
    setFocus();
}

void CanvasView::cancelTextEditor() {
    m_textEditor->hide();
    m_textEditor->clear();
    setFocus();
}

} // namespace agdraw::ui
