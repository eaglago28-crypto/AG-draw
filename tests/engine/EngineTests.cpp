#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include <cmath>

#include "AgdDocumentIO.h"
#include "BitmapTracer.h"
#include "BrushStroke.h"
#include "ColorSeparationExporter.h"
#include "Commands.h"
#include "CompoundPathShape.h"
#include "Document.h"
#include "EllipseShape.h"
#include "Layer.h"
#include "Macro.h"
#include "Page.h"
#include "PathShape.h"
#include "PdfExporter.h"
#include "PowerClipGroup.h"
#include "RectShape.h"
#include "ShapeRecognizer.h"
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
    void contourUndoRedo();
    void interpolateColorMidpoint();
    void agdRoundTripPreservesContour();
    void envelopeUndoRedo();
    void applyEnvelopeIdentityAndWarp();
    void agdRoundTripPreservesEnvelope();
    void extrusionUndoRedo();
    void agdRoundTripPreservesExtrusion();
    void powerClipUndoRestoresOriginalOrder();
    void powerClipClipsHitTestToContainer();
    void agdRoundTripPreservesPowerClip();
    void applyMacroRunsStepsOnEveryTarget();
    void applyMacroUndoesAsOneStep();
    void agdRoundTripPreservesMacros();
    void traceBitmapSingleSquareProducesOneClosedShape();
    void traceBitmapTwoBlobsProduceTwoShapes();
    void traceBitmapBlankImageProducesNothing();
    void traceBitmapIgnoresBlobsBelowMinArea();
    void rgbToCmykConvertsKnownColors();
    void exportColorSeparationsWritesFourGrayscalePngs();
    void exportPdfWritesValidPdfFile();
    void exportPdfWithCropMarksProducesLargerPage();
    void compoundPathBoundsUnionOfContours();
    void compoundPathHoleIsNotContained();
    void compoundPathTranslateMovesAllContours();
    void agdRoundTripPreservesCompoundPath();
    void recognizeShapeDetectsCircleAsEllipse();
    void recognizeShapeDetectsSquareAsRectangle();
    void recognizeShapeRejectsOpenStroke();
    void recognizeShapeRejectsTooFewPoints();
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

void EngineTests::contourUndoRedo() {
    Document doc;
    Shape *rect = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 10, 10)));
    QVERIFY(rect->supportsFillEffects());
    QVERIFY(!rect->contourEnabled);

    doc.undoStack()->push(new SetContourCommand(rect, rect->contourEnabled, true));
    QVERIFY(rect->contourEnabled);

    doc.undoStack()->undo();
    QVERIFY(!rect->contourEnabled);

    doc.undoStack()->redo();
    QVERIFY(rect->contourEnabled);
}

void EngineTests::interpolateColorMidpoint() {
    const QColor result = interpolateColor(Qt::black, Qt::white, 0.5);
    QVERIFY(result.red() >= 126 && result.red() <= 129);
    QVERIFY(result.green() >= 126 && result.green() <= 129);
    QVERIFY(result.blue() >= 126 && result.blue() <= 129);

    QCOMPARE(interpolateColor(Qt::red, Qt::blue, 0.0), QColor(Qt::red));
    QCOMPARE(interpolateColor(Qt::red, Qt::blue, 1.0), QColor(Qt::blue));
}

void EngineTests::agdRoundTripPreservesContour() {
    Document doc;
    auto *rect = static_cast<RectShape *>(doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40))));
    rect->contourEnabled = true;
    rect->contourSteps = 5;
    rect->contourOffset = 6.0;
    rect->contourColor = QColor(10, 200, 30);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("contour.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    Shape *loadedShape = loaded.activeLayer()->shapes().front().get();
    QVERIFY(loadedShape->contourEnabled);
    QCOMPARE(loadedShape->contourSteps, 5);
    QCOMPARE(loadedShape->contourOffset, 6.0);
    QCOMPARE(loadedShape->contourColor, QColor(10, 200, 30));
}

void EngineTests::envelopeUndoRedo() {
    Document doc;
    Shape *rect = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));
    QVERIFY(!rect->envelopeEnabled);
    QVERIFY(rect->envelopeCorners.isEmpty());

    const QVector<QPointF> corners = defaultEnvelopeCorners(rect->bounds());
    doc.undoStack()->push(new SetEnvelopeCommand(rect, rect->envelopeEnabled, true, rect->envelopeCorners, corners));
    QVERIFY(rect->envelopeEnabled);
    QCOMPARE(rect->envelopeCorners, corners);

    doc.undoStack()->undo();
    QVERIFY(!rect->envelopeEnabled);
    QVERIFY(rect->envelopeCorners.isEmpty());

    doc.undoStack()->redo();
    QVERIFY(rect->envelopeEnabled);
    QCOMPARE(rect->envelopeCorners, corners);

    // Glisser une poignée : SetEnvelopeCornersCommand ne touche que les coins.
    QVector<QPointF> dragged = corners;
    dragged[0] = QPointF(5, 5);
    doc.undoStack()->push(new SetEnvelopeCornersCommand(rect, corners, dragged));
    QCOMPARE(rect->envelopeCorners[0], QPointF(5, 5));
    QVERIFY(rect->envelopeEnabled); // inchangé par cette commande

    doc.undoStack()->undo();
    QCOMPARE(rect->envelopeCorners[0], QPointF(0, 0));
}

void EngineTests::applyEnvelopeIdentityAndWarp() {
    const QRectF bounds(0, 0, 10, 10);
    const QVector<QPointF> identityCorners = defaultEnvelopeCorners(bounds);
    QPolygonF source;
    source << QPointF(0, 0) << QPointF(10, 0) << QPointF(10, 10) << QPointF(0, 10) << QPointF(5, 5);

    // Enveloppe non déformée : chaque point reste à sa place.
    const QPolygonF identityResult = applyEnvelope(source, bounds, identityCorners);
    for (int i = 0; i < source.size(); ++i) {
        QVERIFY(std::abs(identityResult[i].x() - source[i].x()) < 1e-9);
        QVERIFY(std::abs(identityResult[i].y() - source[i].y()) < 1e-9);
    }

    // Coin haut-gauche tiré vers l'intérieur : le point (0,0) doit suivre.
    QVector<QPointF> draggedCorners = identityCorners;
    draggedCorners[0] = QPointF(3, 3);
    const QPolygonF draggedResult = applyEnvelope(source, bounds, draggedCorners);
    QCOMPARE(draggedResult.first(), QPointF(3, 3));
    // Le coin bas-droit (opposé), lui, reste fixe.
    QCOMPARE(draggedResult[2], QPointF(10, 10));
    // Le centre (u=v=0.5) est tiré vers la moyenne des 4 coins.
    const QPointF expectedCenter = (draggedCorners[0] + draggedCorners[1] + draggedCorners[2] + draggedCorners[3]) / 4.0;
    QVERIFY(std::abs(draggedResult.last().x() - expectedCenter.x()) < 1e-9);
    QVERIFY(std::abs(draggedResult.last().y() - expectedCenter.y()) < 1e-9);
}

void EngineTests::agdRoundTripPreservesEnvelope() {
    Document doc;
    auto *rect = static_cast<RectShape *>(doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40))));
    rect->envelopeEnabled = true;
    rect->envelopeCorners = {QPointF(2, 2), QPointF(38, -4), QPointF(40, 40), QPointF(0, 40)};

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("envelope.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    Shape *loadedShape = loaded.activeLayer()->shapes().front().get();
    QVERIFY(loadedShape->envelopeEnabled);
    QCOMPARE(loadedShape->envelopeCorners.size(), 4);
    QCOMPARE(loadedShape->envelopeCorners[0], QPointF(2, 2));
    QCOMPARE(loadedShape->envelopeCorners[1], QPointF(38, -4));
    QCOMPARE(loadedShape->envelopeCorners[2], QPointF(40, 40));
    QCOMPARE(loadedShape->envelopeCorners[3], QPointF(0, 40));
}

void EngineTests::extrusionUndoRedo() {
    Document doc;
    Shape *rect = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));
    QVERIFY(!rect->extrusionEnabled);

    doc.undoStack()->push(new SetExtrusionCommand(rect, rect->extrusionEnabled, true));
    QVERIFY(rect->extrusionEnabled);

    doc.undoStack()->undo();
    QVERIFY(!rect->extrusionEnabled);

    doc.undoStack()->redo();
    QVERIFY(rect->extrusionEnabled);
}

void EngineTests::agdRoundTripPreservesExtrusion() {
    Document doc;
    auto *rect = static_cast<RectShape *>(doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40))));
    rect->extrusionEnabled = true;
    rect->extrusionDepth = 15.0;
    rect->extrusionAngle = 30.0;
    rect->extrusionColor = QColor(50, 60, 70);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("extrusion.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    Shape *loadedShape = loaded.activeLayer()->shapes().front().get();
    QVERIFY(loadedShape->extrusionEnabled);
    QCOMPARE(loadedShape->extrusionDepth, 15.0);
    QCOMPARE(loadedShape->extrusionAngle, 30.0);
    QCOMPARE(loadedShape->extrusionColor, QColor(50, 60, 70));
}

void EngineTests::powerClipUndoRestoresOriginalOrder() {
    Document doc;
    Layer *layer = doc.activeLayer();
    Shape *a = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 10, 10)));
    Shape *b = layer->addShape(std::make_unique<RectShape>(QRectF(1, 1, 5, 5)));
    Shape *c = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40)));
    Shape *d = layer->addShape(std::make_unique<RectShape>(QRectF(50, 50, 10, 10)));
    QCOMPARE(layer->shapeCount(), size_t(4));

    // b = contenu, c = contenant (dernier de la liste des membres).
    QVector<Shape *> members{b, c};
    auto *command = new ApplyPowerClipCommand(layer, members, c, QStringLiteral("PowerClip"));
    doc.undoStack()->push(command);

    QCOMPARE(layer->shapeCount(), size_t(3));
    Shape *group = command->groupPtr();
    QVERIFY(group);
    QCOMPARE(layer->indexOf(a), size_t(0));
    QCOMPARE(layer->indexOf(group), size_t(1)); // remplace la place laissée par b et c
    QCOMPARE(layer->indexOf(d), size_t(2));

    doc.undoStack()->undo();
    QCOMPARE(layer->shapeCount(), size_t(4));
    QCOMPARE(layer->indexOf(a), size_t(0));
    QCOMPARE(layer->indexOf(b), size_t(1));
    QCOMPARE(layer->indexOf(c), size_t(2));
    QCOMPARE(layer->indexOf(d), size_t(3));

    // Le redo doit réutiliser le même objet groupe (pointeur stable pour
    // tout code qui l'aurait référencé, comme la sélection courante de l'UI).
    doc.undoStack()->redo();
    QCOMPARE(layer->shapeCount(), size_t(3));
    QCOMPARE(command->groupPtr(), group);
    QCOMPARE(layer->indexOf(group), size_t(1));
}

void EngineTests::powerClipClipsHitTestToContainer() {
    Document doc;
    Layer *layer = doc.activeLayer();
    // Le contenu déborde largement du contenant.
    Shape *content = layer->addShape(std::make_unique<RectShape>(QRectF(-100, -100, 300, 300)));
    Shape *container = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));

    QVector<Shape *> members{content, container};
    doc.undoStack()->push(new ApplyPowerClipCommand(layer, members, container, QStringLiteral("PowerClip")));

    Shape *hit = doc.shapeAt(QPointF(10, 10)); // à l'intérieur du contenant
    QCOMPARE(hit, layer->shapes().front().get());

    Shape *miss = doc.shapeAt(QPointF(200, 200)); // dans le contenu, hors du contenant
    QVERIFY(miss == nullptr);
}

void EngineTests::agdRoundTripPreservesPowerClip() {
    Document doc;
    Layer *layer = doc.activeLayer();
    Shape *content = layer->addShape(std::make_unique<RectShape>(QRectF(2, 2, 6, 6)));
    Shape *container = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));
    QVector<Shape *> members{content, container};
    doc.undoStack()->push(new ApplyPowerClipCommand(layer, members, container, QStringLiteral("PowerClip")));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("powerclip.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    QCOMPARE(loaded.activeLayer()->shapeCount(), size_t(1));
    auto *group = dynamic_cast<PowerClipGroup *>(loaded.activeLayer()->shapes().front().get());
    QVERIFY(group);
    QCOMPARE(group->container()->bounds(), QRectF(0, 0, 20, 20));
    QCOMPARE(group->contents().size(), size_t(1));
    QCOMPARE(group->contents().front()->bounds(), QRectF(2, 2, 6, 6));
}

void EngineTests::applyMacroRunsStepsOnEveryTarget() {
    Document doc;
    Shape *a = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 10, 10)));
    Shape *b = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(20, 20, 10, 10)));

    Macro macro;
    macro.name = QStringLiteral("Style A");
    MacroStep colorStep;
    colorStep.kind = MacroStep::Kind::SetFillColor;
    colorStep.color = QColor(0, 200, 0);
    macro.steps.append(colorStep);
    MacroStep moveStep;
    moveStep.kind = MacroStep::Kind::Translate;
    moveStep.delta = QPointF(10, 5);
    macro.steps.append(moveStep);

    applyMacro(doc, macro, {a, b});

    QCOMPARE(a->fillColor, QColor(0, 200, 0));
    QCOMPARE(b->fillColor, QColor(0, 200, 0));
    QCOMPARE(a->bounds(), QRectF(10, 5, 10, 10));
    QCOMPARE(b->bounds(), QRectF(30, 25, 10, 10));
}

void EngineTests::applyMacroUndoesAsOneStep() {
    Document doc;
    Shape *a = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 10, 10)));
    Shape *b = doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(20, 20, 10, 10)));
    const QColor originalColor = a->fillColor;

    Macro macro;
    macro.name = QStringLiteral("Style A");
    MacroStep colorStep;
    colorStep.kind = MacroStep::Kind::SetFillColor;
    colorStep.color = QColor(0, 200, 0);
    macro.steps.append(colorStep);
    MacroStep moveStep;
    moveStep.kind = MacroStep::Kind::Translate;
    moveStep.delta = QPointF(10, 5);
    macro.steps.append(moveStep);

    const int indexBefore = doc.undoStack()->index();
    applyMacro(doc, macro, {a, b});
    QCOMPARE(doc.undoStack()->index(), indexBefore + 1); // une seule entrée dans la pile

    doc.undoStack()->undo();
    QCOMPARE(a->fillColor, originalColor);
    QCOMPARE(b->bounds(), QRectF(20, 20, 10, 10));
}

void EngineTests::agdRoundTripPreservesMacros() {
    Document doc;
    Macro macro;
    macro.name = QStringLiteral("Style A");
    MacroStep colorStep;
    colorStep.kind = MacroStep::Kind::SetStrokeColor;
    colorStep.color = QColor(10, 20, 30);
    macro.steps.append(colorStep);
    MacroStep shadowStep;
    shadowStep.kind = MacroStep::Kind::SetShadow;
    shadowStep.enabled = true;
    macro.steps.append(shadowStep);
    MacroStep moveStep;
    moveStep.kind = MacroStep::Kind::Translate;
    moveStep.delta = QPointF(3, -4);
    macro.steps.append(moveStep);
    doc.addMacro(macro);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("macros.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    QCOMPARE(loaded.macros().size(), size_t(1));
    const Macro &loadedMacro = loaded.macros().front();
    QCOMPARE(loadedMacro.name, QStringLiteral("Style A"));
    QCOMPARE(loadedMacro.steps.size(), 3);
    QCOMPARE(loadedMacro.steps[0].kind, MacroStep::Kind::SetStrokeColor);
    QCOMPARE(loadedMacro.steps[0].color, QColor(10, 20, 30));
    QCOMPARE(loadedMacro.steps[1].kind, MacroStep::Kind::SetShadow);
    QVERIFY(loadedMacro.steps[1].enabled);
    QCOMPARE(loadedMacro.steps[2].kind, MacroStep::Kind::Translate);
    QCOMPARE(loadedMacro.steps[2].delta, QPointF(3, -4));
}

void EngineTests::traceBitmapSingleSquareProducesOneClosedShape() {
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(Qt::white);
    for (int y = 2; y < 5; ++y) {
        for (int x = 2; x < 5; ++x) {
            image.setPixel(x, y, qRgb(0, 0, 0));
        }
    }

    const auto shapes = traceBitmap(image, QRectF(0, 0, 8, 8));
    QCOMPARE(shapes.size(), size_t(1));

    auto *path = dynamic_cast<PathShape *>(shapes.front().get());
    QVERIFY(path);
    QVERIFY(path->closed);
    QCOMPARE(path->bounds(), QRectF(2, 2, 3, 3));
    QCOMPARE(path->fillColor, QColor(0, 0, 0));
}

void EngineTests::traceBitmapTwoBlobsProduceTwoShapes() {
    QImage image(13, 6, QImage::Format_RGB32);
    image.fill(Qt::white);
    for (int y = 1; y < 4; ++y) {
        for (int x = 1; x < 4; ++x) {
            image.setPixel(x, y, qRgb(0, 0, 0));
        }
    }
    for (int y = 1; y < 4; ++y) {
        for (int x = 8; x < 11; ++x) {
            image.setPixel(x, y, qRgb(200, 20, 20));
        }
    }

    const auto shapes = traceBitmap(image, QRectF(0, 0, 13, 6));
    QCOMPARE(shapes.size(), size_t(2));
    for (const auto &shape : shapes) {
        auto *path = dynamic_cast<PathShape *>(shape.get());
        QVERIFY(path);
        QVERIFY(path->closed);
    }
}

void EngineTests::traceBitmapBlankImageProducesNothing() {
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(Qt::white);

    const auto shapes = traceBitmap(image, QRectF(0, 0, 8, 8));
    QVERIFY(shapes.empty());
}

void EngineTests::traceBitmapIgnoresBlobsBelowMinArea() {
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(Qt::white);
    image.setPixel(0, 0, qRgb(0, 0, 0)); // bruit d'un seul pixel
    for (int y = 4; y < 7; ++y) {
        for (int x = 4; x < 7; ++x) {
            image.setPixel(x, y, qRgb(0, 0, 0));
        }
    }

    const auto shapes = traceBitmap(image, QRectF(0, 0, 8, 8)); // minBlobArea par défaut = 6
    QCOMPARE(shapes.size(), size_t(1));
    auto *path = dynamic_cast<PathShape *>(shapes.front().get());
    QVERIFY(path);
    QCOMPARE(path->bounds(), QRectF(4, 4, 3, 3));
}

void EngineTests::rgbToCmykConvertsKnownColors() {
    const agdraw::io::Cmyk white = agdraw::io::rgbToCmyk(Qt::white);
    QVERIFY(white.c < 1e-9 && white.m < 1e-9 && white.y < 1e-9 && white.k < 1e-9);

    const agdraw::io::Cmyk black = agdraw::io::rgbToCmyk(Qt::black);
    QVERIFY(black.k > 1.0 - 1e-9);
    QVERIFY(black.c < 1e-9 && black.m < 1e-9 && black.y < 1e-9);

    const agdraw::io::Cmyk red = agdraw::io::rgbToCmyk(QColor(255, 0, 0));
    QVERIFY(red.k < 1e-9);
    QVERIFY(red.c < 1e-9);
    QVERIFY(red.m > 1.0 - 1e-9);
    QVERIFY(red.y > 1.0 - 1e-9);
}

void EngineTests::exportColorSeparationsWritesFourGrayscalePngs() {
    Document doc;
    doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40)))->fillColor = QColor(255, 0, 0);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString basePath = dir.filePath("sep");

    QString error;
    QVERIFY2(agdraw::io::exportColorSeparations(doc, basePath, QRectF(0, 0, 40, 40), QSize(40, 40), &error),
              qPrintable(error));

    const QImage cyan(basePath + "_C.png");
    const QImage magenta(basePath + "_M.png");
    const QImage yellow(basePath + "_Y.png");
    const QImage black(basePath + "_K.png");
    QVERIFY(!cyan.isNull() && !magenta.isNull() && !yellow.isNull() && !black.isNull());

    const QPoint center(20, 20);
    QVERIFY(qGray(cyan.pixel(center)) > 240);   // pas d'encre cyan sur du rouge pur
    QVERIFY(qGray(magenta.pixel(center)) < 15); // encre magenta pleine
    QVERIFY(qGray(yellow.pixel(center)) < 15);  // encre jaune pleine
    QVERIFY(qGray(black.pixel(center)) > 240);  // pas de noir sur du rouge pur
}

void EngineTests::exportPdfWritesValidPdfFile() {
    Document doc;
    doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40)));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("out.pdf");

    QString error;
    QVERIFY2(agdraw::io::exportPdf(doc, path, QRectF(0, 0, 100, 100), false, &error), qPrintable(error));

    QFile file(path);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QVERIFY(file.size() > 0);
    QCOMPARE(file.read(5), QByteArray("%PDF-"));
}

void EngineTests::exportPdfWithCropMarksProducesLargerPage() {
    Document doc;
    doc.activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40)));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString plainPath = dir.filePath("plain.pdf");
    const QString marksPath = dir.filePath("marks.pdf");

    QString error;
    QVERIFY2(agdraw::io::exportPdf(doc, plainPath, QRectF(0, 0, 100, 100), false, &error), qPrintable(error));
    QVERIFY2(agdraw::io::exportPdf(doc, marksPath, QRectF(0, 0, 100, 100), true, &error), qPrintable(error));

    QFile plainFile(plainPath);
    QFile marksFile(marksPath);
    QVERIFY(plainFile.open(QIODevice::ReadOnly));
    QVERIFY(marksFile.open(QIODevice::ReadOnly));
    // La page avec repères est agrandie d'une marge de fond perdu : le
    // contenu PDF (donc la taille du fichier) diffère forcément.
    QVERIFY(plainFile.readAll() != marksFile.readAll());
}

void EngineTests::compoundPathBoundsUnionOfContours() {
    QVector<QPointF> squareA{QPointF(0, 0), QPointF(10, 0), QPointF(10, 10), QPointF(0, 10)};
    QVector<QPointF> squareB{QPointF(50, 50), QPointF(60, 50), QPointF(60, 60), QPointF(50, 60)};
    CompoundPathShape shape({squareA, squareB});

    QCOMPARE(shape.bounds(), QRectF(0, 0, 60, 60));
}

void EngineTests::compoundPathHoleIsNotContained() {
    QVector<QPointF> outer{QPointF(0, 0), QPointF(20, 0), QPointF(20, 20), QPointF(0, 20)};
    QVector<QPointF> hole{QPointF(5, 5), QPointF(15, 5), QPointF(15, 15), QPointF(5, 15)};
    CompoundPathShape shape({outer, hole});

    QVERIFY(shape.contains(QPointF(2, 2)));    // dans l'anneau
    QVERIFY(!shape.contains(QPointF(10, 10))); // dans le trou
    QVERIFY(!shape.contains(QPointF(30, 30))); // en dehors
}

void EngineTests::compoundPathTranslateMovesAllContours() {
    QVector<QPointF> squareA{QPointF(0, 0), QPointF(10, 0), QPointF(10, 10), QPointF(0, 10)};
    QVector<QPointF> squareB{QPointF(50, 50), QPointF(60, 50), QPointF(60, 60), QPointF(50, 60)};
    CompoundPathShape shape({squareA, squareB});

    shape.translate(QPointF(5, -3));
    QCOMPARE(shape.contours()[0][0], QPointF(5, -3));
    QCOMPARE(shape.contours()[1][0], QPointF(55, 47));
}

void EngineTests::agdRoundTripPreservesCompoundPath() {
    Document doc;
    QVector<QPointF> outer{QPointF(0, 0), QPointF(20, 0), QPointF(20, 20), QPointF(0, 20)};
    QVector<QPointF> hole{QPointF(5, 5), QPointF(15, 5), QPointF(15, 15), QPointF(5, 15)};
    auto *shape = static_cast<CompoundPathShape *>(
        doc.activeLayer()->addShape(std::make_unique<CompoundPathShape>(std::vector<QVector<QPointF>>{outer, hole})));
    shape->fillColor = QColor(30, 90, 150);

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("compound.agd");

    QString error;
    QVERIFY2(agdraw::io::saveAgd(doc, path, &error), qPrintable(error));

    Document loaded;
    QVERIFY2(agdraw::io::loadAgd(loaded, path, &error), qPrintable(error));

    auto *loadedShape = dynamic_cast<CompoundPathShape *>(loaded.activeLayer()->shapes().front().get());
    QVERIFY(loadedShape);
    QCOMPARE(loadedShape->fillColor, QColor(30, 90, 150));
    QCOMPARE(loadedShape->contours().size(), size_t(2));
    QCOMPARE(loadedShape->contours()[0], outer);
    QCOMPARE(loadedShape->contours()[1], hole);
}

void EngineTests::recognizeShapeDetectsCircleAsEllipse() {
    QVector<QPointF> points;
    const QPointF center(100, 100);
    const qreal radius = 50.0;
    const int n = 40;
    for (int i = 0; i < n; ++i) {
        const qreal angle = 2.0 * M_PI * i / n;
        points.append(QPointF(center.x() + radius * std::cos(angle), center.y() + radius * std::sin(angle)));
    }
    points.append(points.first()); // referme la boucle.

    const ShapeRecognitionResult result = recognizeShape(points);
    QCOMPARE(result.kind, RecognizedShapeKind::Ellipse);
    QVERIFY(result.shape);
    QVERIFY(dynamic_cast<EllipseShape *>(result.shape.get()) != nullptr);
    const QRectF bounds = result.shape->bounds();
    QVERIFY(std::abs(bounds.width() - 2 * radius) < 5.0);
    QVERIFY(std::abs(bounds.height() - 2 * radius) < 5.0);
}

void EngineTests::recognizeShapeDetectsSquareAsRectangle() {
    QVector<QPointF> points;
    constexpr int kPerSide = 10;
    auto addEdge = [&](QPointF a, QPointF b) {
        for (int i = 0; i < kPerSide; ++i) {
            const qreal t = static_cast<qreal>(i) / kPerSide;
            points.append(a + (b - a) * t);
        }
    };
    addEdge(QPointF(0, 0), QPointF(100, 0));
    addEdge(QPointF(100, 0), QPointF(100, 100));
    addEdge(QPointF(100, 100), QPointF(0, 100));
    addEdge(QPointF(0, 100), QPointF(0, 0));
    points.append(points.first()); // referme la boucle.

    const ShapeRecognitionResult result = recognizeShape(points);
    QCOMPARE(result.kind, RecognizedShapeKind::Rectangle);
    QVERIFY(result.shape);
    QVERIFY(dynamic_cast<RectShape *>(result.shape.get()) != nullptr);
    QCOMPARE(result.shape->bounds(), QRectF(0, 0, 100, 100));
}

void EngineTests::recognizeShapeRejectsOpenStroke() {
    QVector<QPointF> points;
    for (int i = 0; i <= 20; ++i) {
        points.append(QPointF(i * 5, i * 5));
    }
    const ShapeRecognitionResult result = recognizeShape(points);
    QCOMPARE(result.kind, RecognizedShapeKind::None);
    QVERIFY(!result.shape);
}

void EngineTests::recognizeShapeRejectsTooFewPoints() {
    const QVector<QPointF> points{QPointF(0, 0), QPointF(10, 10), QPointF(20, 0)};
    const ShapeRecognitionResult result = recognizeShape(points);
    QCOMPARE(result.kind, RecognizedShapeKind::None);
    QVERIFY(!result.shape);
}

QTEST_APPLESS_MAIN(EngineTests)
#include "EngineTests.moc"
