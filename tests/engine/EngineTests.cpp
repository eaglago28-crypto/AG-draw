#include <QTemporaryDir>
#include <QTest>

#include "AgdDocumentIO.h"
#include "Commands.h"
#include "Document.h"
#include "EllipseShape.h"
#include "Layer.h"
#include "PathShape.h"
#include "RectShape.h"
#include "TextShape.h"

using namespace agdraw::engine;

class EngineTests : public QObject {
    Q_OBJECT

private slots:
    void addShapeUndo();
    void translateUndo();
    void resizeUndo();
    void removeUndoRestoresPosition();
    void reorderUndo();
    void shapeAtHitTest();
    void pathBezierBoundsDifferFromStraight();
    void agdRoundTrip();
    void shadowAndGradientUndo();
    void agdRoundTripPreservesEffects();
    void hiddenLockedLayerIgnoredByHitTest();
};

void EngineTests::addShapeUndo() {
    Document doc;
    QCOMPARE(doc.activeLayer()->shapeCount(), size_t(0));

    auto *command = new AddShapeCommand(doc.activeLayer(), std::make_unique<RectShape>(QRectF(0, 0, 10, 10)), "Rectangle");
    doc.undoStack()->push(command);
    QCOMPARE(doc.activeLayer()->shapeCount(), size_t(1));

    doc.undoStack()->undo();
    QCOMPARE(doc.activeLayer()->shapeCount(), size_t(0));

    doc.undoStack()->redo();
    QCOMPARE(doc.activeLayer()->shapeCount(), size_t(1));
}

void EngineTests::translateUndo() {
    Document doc;
    auto *command = new AddShapeCommand(doc.activeLayer(), std::make_unique<RectShape>(QRectF(0, 0, 10, 10)), "Rectangle");
    doc.undoStack()->push(command);
    Shape *shape = command->shapePtr();

    doc.undoStack()->push(new TranslateShapeCommand(shape, QPointF(5, 7)));
    QCOMPARE(shape->bounds().topLeft(), QPointF(5, 7));

    doc.undoStack()->undo();
    QCOMPARE(shape->bounds().topLeft(), QPointF(0, 0));
}

void EngineTests::resizeUndo() {
    Document doc;
    auto *addCommand = new AddShapeCommand(doc.activeLayer(), std::make_unique<RectShape>(QRectF(0, 0, 10, 10)), "Rectangle");
    doc.undoStack()->push(addCommand);
    Shape *shape = addCommand->shapePtr();

    const QRectF oldBounds = shape->bounds();
    const QRectF newBounds(0, 0, 50, 60);
    doc.undoStack()->push(new ResizeShapeCommand(shape, oldBounds, newBounds));
    QCOMPARE(shape->bounds(), newBounds);

    doc.undoStack()->undo();
    QCOMPARE(shape->bounds(), oldBounds);
}

void EngineTests::removeUndoRestoresPosition() {
    Document doc;
    Layer *layer = doc.activeLayer();
    Shape *first = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 10, 10)));
    Shape *second = layer->addShape(std::make_unique<RectShape>(QRectF(20, 0, 10, 10)));
    Shape *third = layer->addShape(std::make_unique<RectShape>(QRectF(40, 0, 10, 10)));
    QCOMPARE(layer->indexOf(second), size_t(1));

    doc.undoStack()->push(new RemoveShapeCommand(layer, second, "Supprimer"));
    QCOMPARE(layer->shapeCount(), size_t(2));

    doc.undoStack()->undo();
    QCOMPARE(layer->shapeCount(), size_t(3));
    QCOMPARE(layer->indexOf(first), size_t(0));
    QCOMPARE(layer->indexOf(second), size_t(1));
    QCOMPARE(layer->indexOf(third), size_t(2));
}

void EngineTests::reorderUndo() {
    Document doc;
    Layer *layer = doc.activeLayer();
    Shape *first = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 10, 10)));
    Shape *second = layer->addShape(std::make_unique<RectShape>(QRectF(20, 0, 10, 10)));
    QCOMPARE(layer->indexOf(first), size_t(0));

    doc.undoStack()->push(new ReorderShapeCommand(layer, 0, 1, "Premier plan"));
    QCOMPARE(layer->indexOf(first), size_t(1));
    QCOMPARE(layer->indexOf(second), size_t(0));

    doc.undoStack()->undo();
    QCOMPARE(layer->indexOf(first), size_t(0));
    QCOMPARE(layer->indexOf(second), size_t(1));
}

void EngineTests::shapeAtHitTest() {
    Document doc;
    Shape *rect = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 100, 100)));
    QCOMPARE(doc.shapeAt(QPointF(50, 50)), rect);
    QVERIFY(doc.shapeAt(QPointF(200, 200)) == nullptr);
}

void EngineTests::pathBezierBoundsDifferFromStraight() {
    const QVector<PathNode> straightNodes = {
        PathNode{QPointF(0, 0), QPointF(0, 0)},
        PathNode{QPointF(100, 0), QPointF(0, 0)},
    };
    PathShape straight(straightNodes);

    const QVector<PathNode> curvedNodes = {
        PathNode{QPointF(0, 0), QPointF(0, -50)},
        PathNode{QPointF(100, 0), QPointF(0, -50)},
    };
    PathShape curved(curvedNodes);

    QVERIFY(curved.bounds().height() > straight.bounds().height());
}

void EngineTests::agdRoundTrip() {
    Document doc;
    Layer *layer = doc.activeLayer();
    layer->addShape(std::make_unique<RectShape>(QRectF(1, 2, 30, 40)));
    layer->addShape(std::make_unique<EllipseShape>(QRectF(5, 6, 20, 25)));
    layer->addShape(std::make_unique<TextShape>(QPointF(10, 10), QStringLiteral("AG Draw")));

    const QVector<PathNode> nodes = {PathNode{QPointF(0, 0), QPointF(0, 0)}, PathNode{QPointF(10, 10), QPointF(2, 2)}};
    layer->addShape(std::make_unique<PathShape>(nodes));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("test.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    QCOMPARE(loaded.layers().size(), size_t(1));
    QCOMPARE(loaded.activeLayer()->shapeCount(), size_t(4));
}

void EngineTests::shadowAndGradientUndo() {
    Document doc;
    Shape *rect = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 10, 10)));
    QVERIFY(rect->supportsFillEffects());
    QVERIFY(!rect->shadowEnabled);
    QVERIFY(!rect->gradientEnabled);

    doc.undoStack()->push(new SetShadowCommand(rect, rect->shadowEnabled, true));
    QVERIFY(rect->shadowEnabled);
    doc.undoStack()->undo();
    QVERIFY(!rect->shadowEnabled);

    doc.undoStack()->push(new SetGradientCommand(rect, rect->gradientEnabled, true));
    QVERIFY(rect->gradientEnabled);
    doc.undoStack()->undo();
    QVERIFY(!rect->gradientEnabled);

    // Le tracé (plume) ne prend pas en charge les effets de remplissage.
    const QVector<PathNode> nodes = {PathNode{QPointF(0, 0), QPointF(0, 0)}, PathNode{QPointF(5, 5), QPointF(0, 0)}};
    PathShape path(nodes);
    QVERIFY(!path.supportsFillEffects());
}

void EngineTests::agdRoundTripPreservesEffects() {
    Document doc;
    Layer *layer = doc.activeLayer();
    auto *rect = static_cast<RectShape *>(layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 30, 30))));
    rect->shadowEnabled = true;
    rect->shadowColor = QColor(10, 20, 30, 200);
    rect->shadowOffset = QPointF(3, 4);
    rect->gradientEnabled = true;
    rect->gradientStartColor = QColor(255, 0, 0);
    rect->gradientEndColor = QColor(0, 0, 255);
    rect->gradientAngle = 45.0;

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("effects.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    Shape *loadedShape = loaded.activeLayer()->shapes().front().get();
    QVERIFY(loadedShape->shadowEnabled);
    QCOMPARE(loadedShape->shadowColor, QColor(10, 20, 30, 200));
    QCOMPARE(loadedShape->shadowOffset, QPointF(3, 4));
    QVERIFY(loadedShape->gradientEnabled);
    QCOMPARE(loadedShape->gradientStartColor, QColor(255, 0, 0));
    QCOMPARE(loadedShape->gradientEndColor, QColor(0, 0, 255));
    QCOMPARE(loadedShape->gradientAngle, 45.0);
}

void EngineTests::hiddenLockedLayerIgnoredByHitTest() {
    Document doc;
    Layer &hiddenLayer = doc.addLayer(QStringLiteral("Caché"));
    Shape *shape = hiddenLayer.addShape(std::make_unique<RectShape>(QRectF(0, 0, 50, 50)));
    QCOMPARE(doc.shapeAt(QPointF(10, 10)), shape);

    hiddenLayer.setVisible(false);
    QVERIFY(doc.shapeAt(QPointF(10, 10)) == nullptr);

    hiddenLayer.setVisible(true);
    hiddenLayer.setLocked(true);
    QVERIFY(doc.shapeAt(QPointF(10, 10)) == nullptr);
}

QTEST_APPLESS_MAIN(EngineTests)
#include "EngineTests.moc"
