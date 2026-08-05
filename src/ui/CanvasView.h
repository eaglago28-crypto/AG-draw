#pragma once

#include <QGraphicsView>
#include <QVector>
#include <memory>

#include "ToolBox.h"

class QLineEdit;

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
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void setupScene();
    void setupTextEditor();
    int hitTestHandle(const QPoint &viewPos) const;
    QRectF computeResizedBounds(const QPointF &scenePos) const;
    void commitTextEditor();
    void cancelTextEditor();

    std::unique_ptr<agdraw::engine::Document> m_document;
    DocumentItem *m_documentItem = nullptr;

    Tool m_activeTool = Tool::Selection;
    qreal m_zoom = 1.0;

    // Glisser en cours : déplacement d'une forme sélectionnée.
    bool m_dragging = false;
    QPointF m_dragStart;
    QPointF m_lastMovePos;
    agdraw::engine::Shape *m_selectedShape = nullptr;

    // Glisser en cours : création d'un rectangle/ellipse (aperçu seulement,
    // la forme n'existe dans le document qu'au relâchement).
    bool m_creatingShape = false;
    QPointF m_dragCurrent;

    // Glisser en cours : redimensionnement via une poignée de coin.
    bool m_resizing = false;
    int m_activeHandle = -1;
    QRectF m_originalBounds;
    QRectF m_pendingBounds;

    bool m_panning = false;
    QPoint m_lastPanPoint;

    QVector<QPointF> m_penPoints;

    QLineEdit *m_textEditor = nullptr;
    QPointF m_textEditorScenePos;
};

} // namespace agdraw::ui
