#include "CanvasView.h"

#include "Commands.h"
#include "Document.h"
#include "DocumentItem.h"
#include "EllipseShape.h"
#include "PathShape.h"
#include "RectShape.h"
#include "TextShape.h"

#include <QGraphicsScene>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QPainter>
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
    if (m_activeTool == Tool::Pen && !m_penPoints.isEmpty()) {
        m_penPoints.clear();
    }
    if (m_textEditor->isVisible()) {
        cancelTextEditor();
    }
    m_activeTool = tool;
    m_selectedShape = nullptr;
    m_documentItem->update();
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

    if (m_selectedShape && m_activeTool == Tool::Selection) {
        const QRectF bounds = m_resizing ? m_pendingBounds : m_selectedShape->bounds();

        QPen handlePen(accent);
        handlePen.setStyle(Qt::DashLine);
        handlePen.setWidth(0);
        painter->setPen(handlePen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(bounds.adjusted(-2, -2, 2, 2));

        if (m_selectedShape->isResizable()) {
            const qreal handleSize = kHandleRadiusPx / std::max(m_zoom, 0.01);
            painter->setBrush(Qt::white);
            painter->setPen(QPen(accent, 0));
            for (const QPointF &corner : {bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()}) {
                painter->drawRect(QRectF(corner - QPointF(handleSize, handleSize) / 2, QSizeF(handleSize, handleSize)));
            }
        }
    }

    if (!m_penPoints.isEmpty()) {
        QPen previewPen(accent);
        previewPen.setStyle(Qt::DashLine);
        previewPen.setWidth(0);
        painter->setPen(previewPen);
        for (int i = 1; i < m_penPoints.size(); ++i) {
            painter->drawLine(m_penPoints[i - 1], m_penPoints[i]);
        }
        painter->setBrush(accent);
        for (const QPointF &point : m_penPoints) {
            painter->drawEllipse(point, 3, 3);
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
    if (!m_selectedShape || !m_selectedShape->isResizable()) {
        return -1;
    }
    const QRectF bounds = m_selectedShape->bounds();
    const QPointF corners[4] = {bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()};
    for (int i = 0; i < 4; ++i) {
        const QPoint handlePos = mapFromScene(corners[i]);
        if ((handlePos - viewPos).manhattanLength() <= kHandleRadiusPx * 2) {
            return i;
        }
    }
    return -1;
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
                m_originalBounds = m_selectedShape->bounds();
                m_pendingBounds = m_originalBounds;
            } else {
                m_selectedShape = m_document->shapeAt(scenePos);
                m_dragging = m_selectedShape != nullptr;
                m_dragStart = scenePos;
                m_lastMovePos = scenePos;
                emit statusMessage(m_selectedShape ? tr("Forme sélectionnée") : tr("Aucune forme sous le curseur"));
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
            m_penPoints.append(scenePos);
            emit statusMessage(tr("Plume : %1 point(s) — double-cliquez pour terminer, Échap pour annuler")
                                    .arg(m_penPoints.size()));
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
        m_selectedShape->setBounds(m_pendingBounds);
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

    if (!m_dragging) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    if (m_activeTool == Tool::Selection && m_selectedShape) {
        m_selectedShape->translate(scenePos - m_lastMovePos);
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
        m_selectedShape->setBounds(m_originalBounds);
        if (finalBounds != m_originalBounds) {
            m_document->undoStack()->push(new engine::ResizeShapeCommand(m_selectedShape, m_originalBounds, finalBounds));
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
            auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), label);
            m_document->undoStack()->push(command);
            m_selectedShape = command->shapePtr();
        }
        m_documentItem->update();
        viewport()->update();
        event->accept();
        return;
    }

    if (m_dragging) {
        m_dragging = false;
        if (m_activeTool == Tool::Selection && m_selectedShape) {
            const QPointF totalDelta = mapToScene(event->pos()) - m_dragStart;
            if (!totalDelta.isNull()) {
                m_selectedShape->translate(-totalDelta);
                m_document->undoStack()->push(new engine::TranslateShapeCommand(m_selectedShape, totalDelta));
            }
        }
        emit statusMessage(tr("Prêt"));
    }

    m_documentItem->update();
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_activeTool == Tool::Pen && m_penPoints.size() >= 2) {
        auto shape = std::make_unique<engine::PathShape>(m_penPoints);
        m_penPoints.clear();
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
    if (event->key() == Qt::Key_Escape && !m_penPoints.isEmpty()) {
        m_penPoints.clear();
        m_documentItem->update();
        emit statusMessage(tr("Tracé annulé"));
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Undo)) {
        m_document->undoStack()->undo();
        m_selectedShape = nullptr;
        m_documentItem->update();
        emit statusMessage(tr("Annulé"));
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Redo)) {
        m_document->undoStack()->redo();
        m_selectedShape = nullptr;
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
