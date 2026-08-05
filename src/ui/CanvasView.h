#pragma once

#include <QColor>
#include <QGraphicsView>
#include <QVector>
#include <memory>

#include "Arrange.h"
#include "PathShape.h"
#include "ToolBox.h"

class QPlainTextEdit;

namespace agdraw::engine {
class Document;
class Layer;
class Page;
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

    void newDocument();
    bool saveToFile(const QString &path, QString *errorMessage = nullptr);
    bool loadFromFile(const QString &path, QString *errorMessage = nullptr);
    bool exportToPng(const QString &path, QString *errorMessage = nullptr);
    bool isEmpty() const;
    void goToPage(agdraw::engine::Page *page);

public slots:
    void setActiveTool(agdraw::ui::Tool tool);
    void setActiveColor(const QColor &color);
    void setSelectionStrokeWidth(double width);
    void setSelectionShadow(bool enabled);
    void setSelectionGradient(bool enabled);
    void alignSelection(agdraw::ui::AlignMode mode);
    void distributeSelection(agdraw::ui::DistributeMode mode);
    void refreshView();

signals:
    void statusMessage(const QString &text);
    void selectionChanged(agdraw::engine::Shape *primary);
    void selectionCountChanged(int count);
    void toolShortcutRequested(agdraw::ui::Tool tool);

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
    QVector<agdraw::engine::Shape *> shapesInRect(const QRectF &rect) const;
    void applyCurrentColor(agdraw::engine::Shape *shape) const;
    void commitTextEditor();
    void cancelTextEditor();
    void setSelection(QVector<agdraw::engine::Shape *> newSelection);
    void reorderSelection(bool forward, bool toExtreme);

    std::unique_ptr<agdraw::engine::Document> m_document;
    DocumentItem *m_documentItem = nullptr;

    Tool m_activeTool = Tool::Selection;
    qreal m_zoom = 1.0;
    QColor m_currentColor = QColor(200, 205, 215);
    bool m_colorExplicitlySet = false;

    // Sélection courante (multi-sélection via Maj+clic ou lasso).
    QVector<agdraw::engine::Shape *> m_selection;

    // Glisser en cours : déplacement de la sélection.
    bool m_dragging = false;
    QPointF m_dragStart;
    QPointF m_lastMovePos;

    // Glisser en cours : sélection au lasso (clic sur zone vide).
    bool m_rubberBanding = false;
    QPointF m_rubberBandStart;
    QRectF m_rubberBandRect;

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

    // Plume : nœuds déjà posés (point + poignée optionnelle pour une
    // courbe de Bézier) en attente de validation par double-clic.
    QVector<agdraw::engine::PathNode> m_penNodes;
    bool m_penDraggingHandle = false;

    QPlainTextEdit *m_textEditor = nullptr;
    QPointF m_textEditorScenePos;
};

} // namespace agdraw::ui
