#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QTemporaryDir>
#include <QTest>
#include <QToolBar>
#include <QToolButton>

#include "CanvasView.h"
#include "Document.h"
#include "Layer.h"
#include "LayersPanel.h"
#include "MainWindow.h"
#include "PropertiesBar.h"
#include "RectShape.h"
#include "ToolBox.h"

using namespace agdraw::ui;
using namespace agdraw::engine;

namespace {
// Les outils sont créés dans cet ordre par ToolBox (voir ToolBox.cpp).
constexpr int kSelectionIndex = 0;
constexpr int kRectangleIndex = 1;
} // namespace

class UiTests : public QObject {
    Q_OBJECT

private slots:
    void toolShortcutSwitchesActiveTool();
    void zOrderShortcutsReorderShapes();
    void propertiesBarEditsStrokeWidth();
    void propertiesBarTogglesShadowAndGradient();
    void layersPanelTogglesVisibility();
    void fileRoundTripThroughCanvas();
};

void UiTests::toolShortcutSwitchesActiveTool() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    QVERIFY(canvas);
    canvas->setFocus();

    // Appuyer sur "R" doit activer l'outil Rectangle (raccourci clavier),
    // vérifié en dessinant une forme et en observant le modèle.
    QTest::keyClick(canvas, Qt::Key_R);
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseMove(canvas->viewport(), QPoint(200, 180));
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(200, 180));

    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(1));
    const auto &shapes = canvas->document().activeLayer()->shapes();
    QVERIFY(dynamic_cast<RectShape *>(shapes.front().get()) != nullptr);
}

void UiTests::zOrderShortcutsReorderShapes() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    QVERIFY(canvas && toolbox);
    canvas->setFocus();

    Layer *layer = canvas->document().activeLayer();
    Shape *first = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 50, 50)));
    Shape *second = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 50, 50)));
    QCOMPARE(layer->indexOf(first), size_t(0));
    QCOMPARE(layer->indexOf(second), size_t(1));

    // Les deux rectangles se superposent : un clic sélectionne la forme du
    // dessus (`second`, déjà au premier plan). On l'envoie à l'arrière-plan
    // avec Maj+[ et on vérifie que l'ordre s'inverse.
    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(canvas->mapFromScene(QPointF(25, 25))));
    QTest::keyClick(canvas, Qt::Key_BracketLeft, Qt::ShiftModifier);

    QCOMPARE(layer->indexOf(second), size_t(0));
    QCOMPARE(layer->indexOf(first), size_t(1));
}

void UiTests::propertiesBarEditsStrokeWidth() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    auto *propertiesBar = window.findChild<PropertiesBar *>("PropertiesBar");
    QVERIFY(canvas && toolbox && propertiesBar);
    canvas->setFocus();

    toolbox->actions()[kRectangleIndex]->trigger();
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseMove(canvas->viewport(), QPoint(200, 180));
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(200, 180));

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(150, 140));

    auto *spinBox = propertiesBar->findChild<QDoubleSpinBox *>();
    QVERIFY(spinBox);
    QVERIFY(spinBox->isEnabled());
    QCOMPARE(spinBox->value(), 1.0);

    spinBox->setValue(8.5);
    Shape *shape = canvas->document().activeLayer()->shapes().front().get();
    QCOMPARE(shape->strokeWidth, 8.5);

    canvas->document().undoStack()->undo();
    QCOMPARE(shape->strokeWidth, 1.0);
}

void UiTests::propertiesBarTogglesShadowAndGradient() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    auto *propertiesBar = window.findChild<PropertiesBar *>("PropertiesBar");
    QVERIFY(canvas && toolbox && propertiesBar);
    canvas->setFocus();

    toolbox->actions()[kRectangleIndex]->trigger();
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseMove(canvas->viewport(), QPoint(200, 180));
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(200, 180));

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(150, 140));

    const auto checkBoxes = propertiesBar->findChildren<QCheckBox *>();
    QCOMPARE(checkBoxes.size(), 2);
    QCheckBox *shadowCheck = checkBoxes[0];
    QCheckBox *gradientCheck = checkBoxes[1];
    QVERIFY(shadowCheck->isEnabled());
    QVERIFY(gradientCheck->isEnabled());

    Shape *shape = canvas->document().activeLayer()->shapes().front().get();
    QVERIFY(!shape->shadowEnabled);
    QVERIFY(!shape->gradientEnabled);

    shadowCheck->setChecked(true);
    QVERIFY(shape->shadowEnabled);
    canvas->document().undoStack()->undo();
    QVERIFY(!shape->shadowEnabled);

    gradientCheck->setChecked(true);
    QVERIFY(shape->gradientEnabled);
    canvas->document().undoStack()->undo();
    QVERIFY(!shape->gradientEnabled);
}

void UiTests::layersPanelTogglesVisibility() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *layersPanel = window.findChild<LayersPanel *>("LayersPanel");
    QVERIFY(canvas && layersPanel);

    Layer *layer = canvas->document().activeLayer();
    layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 50, 50)));
    QCOMPARE(canvas->document().shapeAt(QPointF(10, 10)), layer->shapes().front().get());

    // Identifié par son infobulle (voir LayersPanel::refresh) plutôt que par
    // position, pour ne pas dépendre de l'ordre des enfants Qt (le bouton
    // "+ Calque" de la barre d'outils est aussi un QToolButton).
    QToolButton *visibilityButton = nullptr;
    for (QToolButton *button : layersPanel->findChildren<QToolButton *>()) {
        if (button->toolTip() == QObject::tr("Visibilité du calque")) {
            visibilityButton = button;
            break;
        }
    }
    QVERIFY(visibilityButton);
    QVERIFY(visibilityButton->isChecked());

    visibilityButton->setChecked(false);
    QVERIFY(!layer->isVisible());
    QVERIFY(canvas->document().shapeAt(QPointF(10, 10)) == nullptr);

    visibilityButton->setChecked(true);
    QVERIFY(layer->isVisible());
    QVERIFY(canvas->document().shapeAt(QPointF(10, 10)) != nullptr);
}

void UiTests::fileRoundTripThroughCanvas() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    QVERIFY(canvas);

    canvas->document().activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40)));
    canvas->document().activeLayer()->addShape(std::make_unique<RectShape>(QRectF(50, 0, 40, 40)));
    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(2));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("roundtrip.agd");

    QString error;
    QVERIFY2(canvas->saveToFile(path, &error), qPrintable(error));

    canvas->newDocument();
    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(0));

    QVERIFY2(canvas->loadFromFile(path, &error), qPrintable(error));
    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(2));
}

QTEST_MAIN(UiTests)
#include "UiTests.moc"
