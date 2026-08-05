#include <QTemporaryDir>
#include <QTest>

#include "AgdDocumentIO.h"
#include "BrushStroke.h"
#include "Commands.h"
#include "Document.h"
#include "EllipseShape.h"
#include "Layer.h"
#include "Page.h"
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
    void agdRoundTripPreservesMultiLineText();
    void documentHasOneDefaultPage();
    void addPageUndoRedo();
    void removePageUndoRestoresPosition();
    void agdRoundTripPreservesPages();
    void brushOutlineWidensWithPressure();
    void brushStrokeTranslateMovesAllPoints();
    void agdRoundTripPreservesBrushStroke();
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

void EngineTests::agdRoundTripPreservesMultiLineText() {
    Document doc;
    doc.activeLayer()->addShape(std::make_unique<TextShape>(QPointF(5, 10), QStringLiteral("Ligne 1\nLigne 2\nLigne 3")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("multiline.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    auto *text = dynamic_cast<TextShape *>(loaded.activeLayer()->shapes().front().get());
    QVERIFY(text);
    QCOMPARE(text->text, QStringLiteral("Ligne 1\nLigne 2\nLigne 3"));
}

void EngineTests::documentHasOneDefaultPage() {
    Document doc;
    QCOMPARE(doc.pages().size(), size_t(1));
    QVERIFY(doc.activePage() != nullptr);
    QCOMPARE(doc.activePage()->name(), QStringLiteral("Page 1"));
    QCOMPARE(doc.activePage()->rect(), QRectF(0, 0, 794, 1123));
}

void EngineTests::addPageUndoRedo() {
    Document doc;
    QCOMPARE(doc.pages().size(), size_t(1));

    auto *command = new AddPageCommand(&doc, std::make_unique<Page>(QStringLiteral("Page 2"), QRectF(0, 1200, 794, 1123)),
                                        QStringLiteral("Page"));
    doc.undoStack()->push(command);
    QCOMPARE(doc.pages().size(), size_t(2));
    QCOMPARE(doc.activePage(), command->pagePtr());

    doc.undoStack()->undo();
    QCOMPARE(doc.pages().size(), size_t(1));

    doc.undoStack()->redo();
    QCOMPARE(doc.pages().size(), size_t(2));
}

void EngineTests::removePageUndoRestoresPosition() {
    Document doc;
    Page *first = doc.activePage();
    Page &second = doc.addPage(QStringLiteral("Page 2"), QRectF(0, 1200, 794, 1123));
    Page &third = doc.addPage(QStringLiteral("Page 3"), QRectF(0, 2400, 794, 1123));
    QCOMPARE(doc.indexOfPage(&second), size_t(1));

    doc.undoStack()->push(new RemovePageCommand(&doc, &second, QStringLiteral("Supprimer")));
    QCOMPARE(doc.pages().size(), size_t(2));

    doc.undoStack()->undo();
    QCOMPARE(doc.pages().size(), size_t(3));
    QCOMPARE(doc.indexOfPage(first), size_t(0));
    QCOMPARE(doc.indexOfPage(&second), size_t(1));
    QCOMPARE(doc.indexOfPage(&third), size_t(2));
}

void EngineTests::agdRoundTripPreservesPages() {
    Document doc;
    doc.clearPages();
    doc.addPage(QStringLiteral("Couverture"), QRectF(0, 0, 794, 1123));
    doc.addPage(QStringLiteral("Intérieur"), QRectF(0, 1200, 1000, 700));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("pages.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    QCOMPARE(loaded.pages().size(), size_t(2));
    QCOMPARE(loaded.pages()[0]->name(), QStringLiteral("Couverture"));
    QCOMPARE(loaded.pages()[0]->rect(), QRectF(0, 0, 794, 1123));
    QCOMPARE(loaded.pages()[1]->name(), QStringLiteral("Intérieur"));
    QCOMPARE(loaded.pages()[1]->rect(), QRectF(0, 1200, 1000, 700));
}

void EngineTests::brushOutlineWidensWithPressure() {
    const QVector<BrushPoint> lightPoints = {BrushPoint{QPointF(0, 0), 0.2}, BrushPoint{QPointF(100, 0), 0.2}};
    const QVector<BrushPoint> heavyPoints = {BrushPoint{QPointF(0, 0), 1.0}, BrushPoint{QPointF(100, 0), 1.0}};

    BrushStroke light(lightPoints, 10.0);
    BrushStroke heavy(heavyPoints, 10.0);

    QVERIFY(heavy.bounds().height() > light.bounds().height());
}

void EngineTests::brushStrokeTranslateMovesAllPoints() {
    const QVector<BrushPoint> points = {BrushPoint{QPointF(0, 0), 1.0}, BrushPoint{QPointF(10, 10), 1.0}};
    BrushStroke stroke(points);
    stroke.translate(QPointF(5, 5));

    QCOMPARE(stroke.points[0].point, QPointF(5, 5));
    QCOMPARE(stroke.points[1].point, QPointF(15, 15));
}

void EngineTests::agdRoundTripPreservesBrushStroke() {
    Document doc;
    const QVector<BrushPoint> points = {
        BrushPoint{QPointF(0, 0), 0.3},
        BrushPoint{QPointF(20, 5), 0.9},
        BrushPoint{QPointF(40, 0), 0.5},
    };
    doc.activeLayer()->addShape(std::make_unique<BrushStroke>(points, 12.0));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("brush.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    auto *brush = dynamic_cast<BrushStroke *>(loaded.activeLayer()->shapes().front().get());
    QVERIFY(brush);
    QCOMPARE(brush->baseWidth, 12.0);
    QCOMPARE(brush->points.size(), 3);
    QCOMPARE(brush->points[1].point, QPointF(20, 5));
    QCOMPARE(brush->points[1].pressure, 0.9);
}

QTEST_APPLESS_MAIN(EngineTests)
#include "EngineTests.moc"
