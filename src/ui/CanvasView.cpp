#include "CanvasView.h"

#include "Document.h"
#include "DocumentItem.h"
#include "EllipseShape.h"
#include "PathShape.h"
#include "RectShape.h"

#include <QGraphicsScene>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

namespace agdraw::ui {

namespace {
constexpr qreal kSceneExtent = 100000.0;
constexpr qreal kGridStep = 50.0;
constexpr qreal kMinZoom = 0.05;
constexpr qreal kMaxZoom = 40.0;
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

void CanvasView::setActiveTool(Tool tool) {
    if (m_activeTool == Tool::Pen && !m_penPoints.isEmpty()) {
        m_penPoints.clear();
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
    if (m_selectedShape && m_activeTool == Tool::Selection) {
        QPen handlePen(QColor(0, 122, 255));
        handlePen.setStyle(Qt::DashLine);
        handlePen.setWidth(0);
        painter->setPen(handlePen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(m_selectedShape->bounds().adjusted(-2, -2, 2, 2));
    }

    if (!m_penPoints.isEmpty()) {
        QPen previewPen(QColor(0, 122, 255));
        previewPen.setStyle(Qt::DashLine);
        previewPen.setWidth(0);
        painter->setPen(previewPen);
        for (int i = 1; i < m_penPoints.size(); ++i) {
            painter->drawLine(m_penPoints[i - 1], m_penPoints[i]);
        }
        painter->setBrush(QColor(0, 122, 255));
        for (const QPointF &point : m_penPoints) {
            painter->drawEllipse(point, 3, 3);
        }
    }
}

void CanvasView::mousePressEvent(QMouseEvent *event) {
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
    engine::Layer *activeLayer = m_document->activeLayer();

    switch (m_activeTool) {
        case Tool::Selection:
            m_selectedShape = m_document->shapeAt(scenePos);
            m_dragging = m_selectedShape != nullptr;
            m_lastMovePos = scenePos;
            emit statusMessage(m_selectedShape ? tr("Forme sélectionnée") : tr("Aucune forme sous le curseur"));
            break;
        case Tool::Rectangle:
            m_previewShape = activeLayer->addShape(std::make_unique<engine::RectShape>(QRectF(scenePos, scenePos)));
            m_dragStart = scenePos;
            m_dragging = true;
            break;
        case Tool::Ellipse:
            m_previewShape = activeLayer->addShape(std::make_unique<engine::EllipseShape>(QRectF(scenePos, scenePos)));
            m_dragStart = scenePos;
            m_dragging = true;
            break;
        case Tool::Pen:
            m_penPoints.append(scenePos);
            emit statusMessage(tr("Plume : %1 point(s) — double-cliquez pour terminer, Échap pour annuler")
                                    .arg(m_penPoints.size()));
            break;
        case Tool::Text:
            emit statusMessage(tr("Outil texte : arrive avec le module Texte (prochaine étape)"));
            break;
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

    if (!m_dragging) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    const QPointF scenePos = mapToScene(event->pos());

    switch (m_activeTool) {
        case Tool::Selection:
            if (m_selectedShape) {
                m_selectedShape->translate(scenePos - m_lastMovePos);
                m_lastMovePos = scenePos;
            }
            break;
        case Tool::Rectangle:
        case Tool::Ellipse:
            if (m_previewShape) {
                m_previewShape->setBounds(QRectF(m_dragStart, scenePos));
            }
            break;
        default:
            break;
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

    if (m_dragging) {
        m_dragging = false;
        m_previewShape = nullptr;
        emit statusMessage(tr("Prêt"));
    }

    m_documentItem->update();
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_activeTool == Tool::Pen && m_penPoints.size() >= 2) {
        m_document->activeLayer()->addShape(std::make_unique<engine::PathShape>(m_penPoints));
        m_penPoints.clear();
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
    QGraphicsView::keyPressEvent(event);
}

} // namespace agdraw::ui
