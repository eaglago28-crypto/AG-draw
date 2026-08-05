#include "CanvasView.h"

#include <QGraphicsScene>
#include <QWheelEvent>
#include <QPainter>

namespace agdraw::ui {

namespace {
constexpr qreal kSceneExtent = 100000.0;
constexpr qreal kGridStep = 50.0;
constexpr qreal kMinZoom = 0.05;
constexpr qreal kMaxZoom = 40.0;
} // namespace

CanvasView::CanvasView(QWidget *parent) : QGraphicsView(parent) {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setupScene();
}

void CanvasView::setupScene() {
    auto *scene = new QGraphicsScene(-kSceneExtent, -kSceneExtent,
                                      2 * kSceneExtent, 2 * kSceneExtent, this);
    setScene(scene);

    // Page de travail par défaut (A4 à 96 DPI), pour donner un repère visuel
    // au centre de la zone infinie.
    scene->addRect(0, 0, 794, 1123, QPen(Qt::NoPen), QBrush(Qt::white));
    centerOn(794 / 2.0, 1123 / 2.0);
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

} // namespace agdraw::ui
