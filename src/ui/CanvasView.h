#pragma once

#include <QGraphicsView>
#include <QVector>
#include <memory>

#include "ToolBox.h"

namespace agdraw::engine {
class Document;
class Shape;
}

namespace agdraw::ui {

class DocumentItem;

// Zone de dessin infinie. Affiche le document via le moteur (agdraw::engine)
// et traduit les événements souris en opérations sur le modèle, selon
// l'outil actif. Le rendu passe par QPainter aujourd'hui ; Skia pourra
// remplacer DocumentItem::paint sans changer cette classe.
class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CanvasView(QWidget *parent = nullptr);
    ~CanvasView() override;

    agdraw::engine::Document &document();

public slots:
    void setActiveTool(agdraw::ui::Tool tool);

signals:
    void statusMessage(const QString &text);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void setupScene();

    std::unique_ptr<agdraw::engine::Document> m_document;
    DocumentItem *m_documentItem = nullptr;

    Tool m_activeTool = Tool::Selection;
    qreal m_zoom = 1.0;

    bool m_dragging = false;
    QPointF m_dragStart;
    QPointF m_lastMovePos;
    agdraw::engine::Shape *m_previewShape = nullptr;
    agdraw::engine::Shape *m_selectedShape = nullptr;

    bool m_panning = false;
    QPoint m_lastPanPoint;

    QVector<QPointF> m_penPoints;
};

} // namespace agdraw::ui
