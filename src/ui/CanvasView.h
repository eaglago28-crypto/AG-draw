#pragma once

#include <QGraphicsView>

namespace agdraw::ui {

// Zone de dessin infinie. Rendu Qt/QGraphicsScene provisoire ;
// sera remplacé par le moteur Skia à l'Étape 2.
class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CanvasView(QWidget *parent = nullptr);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    void setupScene();

    qreal m_zoom = 1.0;
};

} // namespace agdraw::ui
