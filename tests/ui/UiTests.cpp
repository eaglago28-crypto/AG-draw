#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QFile>
#include <QImage>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QTemporaryDir>
#include <QTest>
#include <QToolBar>
#include <QToolButton>

#include "BrushStroke.h"
#include "CanvasView.h"
#include "Document.h"
#include "Layer.h"
#include "LayersPanel.h"
#include "Macro.h"
#include "MacroPanel.h"
#include "MainWindow.h"
#include "Page.h"
#include "PagesPanel.h"
#include "PathShape.h"
#include "PowerClipGroup.h"
#include "PropertiesBar.h"
#include "RectShape.h"
#include "TextShape.h"
#include "ToolBox.h"

using namespace agdraw::ui;
using namespace agdraw::engine;

namespace {
// Les outils sont créés dans cet ordre par ToolBox (voir ToolBox.cpp).
constexpr int kSelectionIndex = 0;
constexpr int kRectangleIndex = 1;
constexpr int kTextIndex = 3;
constexpr int kBrushIndex = 5;
} // namespace

class UiTests : public QObject {
    Q_OBJECT

private slots:
    void toolShortcutSwitchesActiveTool();
    void zOrderShortcutsReorderShapes();
    void propertiesBarEditsStrokeWidth();
    void propertiesBarTogglesAllShapeEffects();
    void layersPanelTogglesVisibility();
    void alignSelectionAligns();
    void distributeSelectionSpacesEvenly();
    void blendButtonCreatesIntermediateShapes();
    void envelopeHandleDragWarpsCorner();
    void powerClipButtonGroupsSelection();
    void macroRecordingCapturesAndReplaysSteps();
    void traceImageFileAddsClosedShapesFilledByPalette();
    void multiLineTextBoundsTallerThanSingleLine();
    void textToolCreatesMultiLineShape();
    void pagesPanelAddsAndNavigatesPages();
    void newDocumentResetsToOnePage();
    void brushToolCreatesStrokeWithMouse();
    void fileRoundTripThroughCanvas();
    void exportPdfAndSeparationsThroughCanvas();
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

void UiTests::propertiesBarTogglesAllShapeEffects() {
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
    QCOMPARE(checkBoxes.size(), 5);
    QCheckBox *shadowCheck = checkBoxes[0];
    QCheckBox *gradientCheck = checkBoxes[1];
    QCheckBox *contourCheck = checkBoxes[2];
    QCheckBox *envelopeCheck = checkBoxes[3];
    QCheckBox *extrusionCheck = checkBoxes[4];
    QVERIFY(shadowCheck->isEnabled());
    QVERIFY(gradientCheck->isEnabled());
    QVERIFY(contourCheck->isEnabled());
    QVERIFY(envelopeCheck->isEnabled());
    QVERIFY(extrusionCheck->isEnabled());

    Shape *shape = canvas->document().activeLayer()->shapes().front().get();
    QVERIFY(!shape->shadowEnabled);
    QVERIFY(!shape->gradientEnabled);
    QVERIFY(!shape->contourEnabled);
    QVERIFY(!shape->envelopeEnabled);
    QVERIFY(!shape->extrusionEnabled);

    shadowCheck->setChecked(true);
    QVERIFY(shape->shadowEnabled);
    canvas->document().undoStack()->undo();
    QVERIFY(!shape->shadowEnabled);

    gradientCheck->setChecked(true);
    QVERIFY(shape->gradientEnabled);
    canvas->document().undoStack()->undo();
    QVERIFY(!shape->gradientEnabled);

    contourCheck->setChecked(true);
    QVERIFY(shape->contourEnabled);
    canvas->document().undoStack()->undo();
    QVERIFY(!shape->contourEnabled);

    envelopeCheck->setChecked(true);
    QVERIFY(shape->envelopeEnabled);
    QCOMPARE(shape->envelopeCorners.size(), 4); // initialisés aux coins de bounds()
    canvas->document().undoStack()->undo();
    QVERIFY(!shape->envelopeEnabled);

    extrusionCheck->setChecked(true);
    QVERIFY(shape->extrusionEnabled);
    canvas->document().undoStack()->undo();
    QVERIFY(!shape->extrusionEnabled);
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

void UiTests::alignSelectionAligns() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    QVERIFY(canvas && toolbox);
    canvas->setFocus();

    Layer *layer = canvas->document().activeLayer();
    Shape *a = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));
    Shape *b = layer->addShape(std::make_unique<RectShape>(QRectF(100, 50, 20, 20)));

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, canvas->mapFromScene(QPointF(10, 10)));
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::ShiftModifier, canvas->mapFromScene(QPointF(110, 60)));

    canvas->alignSelection(AlignMode::Left);
    QCOMPARE(a->bounds().left(), b->bounds().left());

    canvas->document().undoStack()->undo();
    QCOMPARE(a->bounds().left(), 0.0);
    QCOMPARE(b->bounds().left(), 100.0);
}

void UiTests::distributeSelectionSpacesEvenly() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    auto *propertiesBar = window.findChild<PropertiesBar *>("PropertiesBar");
    QVERIFY(canvas && toolbox && propertiesBar);
    canvas->setFocus();

    Layer *layer = canvas->document().activeLayer();
    Shape *a = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));
    Shape *b = layer->addShape(std::make_unique<RectShape>(QRectF(50, 0, 20, 20))); // pas espacé également
    Shape *c = layer->addShape(std::make_unique<RectShape>(QRectF(200, 0, 20, 20)));

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, canvas->mapFromScene(QPointF(10, 10)));
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::ShiftModifier, canvas->mapFromScene(QPointF(60, 10)));
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::ShiftModifier, canvas->mapFromScene(QPointF(210, 10)));

    // Vérifie le câblage réel du bouton (identifié par son infobulle), pas
    // seulement l'appel direct au slot.
    QToolButton *distributeButton = nullptr;
    for (QToolButton *button : propertiesBar->findChildren<QToolButton *>()) {
        if (button->toolTip() == QObject::tr("Distribuer horizontalement")) {
            distributeButton = button;
            break;
        }
    }
    QVERIFY(distributeButton);
    QVERIFY(distributeButton->isEnabled());
    QTest::mouseClick(distributeButton, Qt::LeftButton);

    const qreal centerA = a->bounds().center().x();
    const qreal centerB = b->bounds().center().x();
    const qreal centerC = c->bounds().center().x();
    QCOMPARE(centerB - centerA, centerC - centerB);
}

void UiTests::blendButtonCreatesIntermediateShapes() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    auto *propertiesBar = window.findChild<PropertiesBar *>("PropertiesBar");
    QVERIFY(canvas && toolbox && propertiesBar);
    canvas->setFocus();

    Layer *layer = canvas->document().activeLayer();
    Shape *a = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));
    Shape *b = layer->addShape(std::make_unique<RectShape>(QRectF(100, 0, 20, 20)));
    a->fillColor = Qt::black;
    b->fillColor = Qt::white;

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, canvas->mapFromScene(QPointF(10, 10)));

    QToolButton *blendButton = nullptr;
    for (QToolButton *button : propertiesBar->findChildren<QToolButton *>()) {
        if (button->text() == QObject::tr("Fondu")) {
            blendButton = button;
            break;
        }
    }
    QVERIFY(blendButton);
    QVERIFY(!blendButton->isEnabled()); // une seule forme sélectionnée pour l'instant

    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::ShiftModifier, canvas->mapFromScene(QPointF(110, 10)));
    QVERIFY(blendButton->isEnabled());

    QTest::mouseClick(blendButton, Qt::LeftButton);

    QCOMPARE(layer->shapeCount(), size_t(7)); // a, b, + 5 formes intermédiaires
    // Le fondu progresse de la couleur la plus sombre (a) vers la plus claire (b).
    for (size_t i = 2; i < layer->shapeCount() - 1; ++i) {
        QVERIFY(layer->shapes()[i]->fillColor.red() < layer->shapes()[i + 1]->fillColor.red());
    }

    // Un seul undo doit retirer les 5 formes générées (poussées dans une macro).
    canvas->document().undoStack()->undo();
    QCOMPARE(layer->shapeCount(), size_t(2));
    QCOMPARE(a->bounds(), QRectF(0, 0, 20, 20));
    QCOMPARE(b->bounds(), QRectF(100, 0, 20, 20));
}

void UiTests::envelopeHandleDragWarpsCorner() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    auto *propertiesBar = window.findChild<PropertiesBar *>("PropertiesBar");
    QVERIFY(canvas && toolbox && propertiesBar);
    canvas->setFocus();

    Layer *layer = canvas->document().activeLayer();
    Shape *shape = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 100, 100)));

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, canvas->mapFromScene(QPointF(50, 50)));

    QCheckBox *envelopeCheck = nullptr;
    for (QCheckBox *box : propertiesBar->findChildren<QCheckBox *>()) {
        if (box->text() == QObject::tr("Enveloppe")) {
            envelopeCheck = box;
            break;
        }
    }
    QVERIFY(envelopeCheck);
    envelopeCheck->setChecked(true);
    QVERIFY(shape->envelopeEnabled);
    QCOMPARE(shape->envelopeCorners[0], QPointF(0, 0)); // haut-gauche, initialisé aux coins de bounds()

    // Glisser la poignée haut-gauche vers l'intérieur (déformation en pointe).
    const QPoint handlePos = canvas->mapFromScene(QPointF(0, 0));
    const QPoint targetPos = canvas->mapFromScene(QPointF(40, 40));
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, handlePos);
    QTest::mouseMove(canvas->viewport(), targetPos);
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, targetPos);

    QCOMPARE(shape->envelopeCorners[0], QPointF(40, 40));
    QCOMPARE(shape->envelopeCorners[2], QPointF(100, 100)); // les autres coins restent inchangés

    canvas->document().undoStack()->undo();
    QCOMPARE(shape->envelopeCorners[0], QPointF(0, 0));
}

void UiTests::powerClipButtonGroupsSelection() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    auto *propertiesBar = window.findChild<PropertiesBar *>("PropertiesBar");
    QVERIFY(canvas && toolbox && propertiesBar);
    canvas->setFocus();

    Layer *layer = canvas->document().activeLayer();
    // Le contenu déborde largement du contenant, pour vérifier le
    // découpage visuellement (et via le hit-test) après l'application.
    Shape *content = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 200, 200)));
    Shape *container = layer->addShape(std::make_unique<RectShape>(QRectF(300, 300, 50, 50)));
    // Éloigné du contenant pour que le clic de sélection ne touche que le contenu.
    content->translate(QPointF(300, -300));
    QCOMPARE(content->bounds(), QRectF(300, -300, 200, 200));

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, canvas->mapFromScene(QPointF(310, -290)));
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::ShiftModifier, canvas->mapFromScene(QPointF(310, 310)));

    QToolButton *powerClipButton = nullptr;
    for (QToolButton *button : propertiesBar->findChildren<QToolButton *>()) {
        if (button->text() == QObject::tr("PowerClip")) {
            powerClipButton = button;
            break;
        }
    }
    QVERIFY(powerClipButton);
    QVERIFY(powerClipButton->isEnabled());
    QTest::mouseClick(powerClipButton, Qt::LeftButton);

    QCOMPARE(layer->shapeCount(), size_t(1));
    auto *group = dynamic_cast<PowerClipGroup *>(layer->shapes().front().get());
    QVERIFY(group);
    QCOMPARE(group->container(), container);
    QCOMPARE(group->contents().size(), size_t(1));
    QCOMPARE(group->contents().front().get(), content);
    QCOMPARE(group->bounds(), QRectF(300, 300, 50, 50));

    canvas->document().undoStack()->undo();
    QCOMPARE(layer->shapeCount(), size_t(2));
}

void UiTests::macroRecordingCapturesAndReplaysSteps() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    auto *macroPanel = window.findChild<MacroPanel *>("MacroPanel");
    QVERIFY(canvas && toolbox && macroPanel);
    canvas->setFocus();

    Layer *layer = canvas->document().activeLayer();
    Shape *recorded = layer->addShape(std::make_unique<RectShape>(QRectF(0, 0, 20, 20)));

    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, canvas->mapFromScene(QPointF(10, 10)));

    QVERIFY(!canvas->isRecordingMacro());
    canvas->startMacroRecording();
    QVERIFY(canvas->isRecordingMacro());

    canvas->setSelectionShadow(true);
    QVERIFY(recorded->shadowEnabled);

    canvas->stopMacroRecording(QStringLiteral("Style A"));
    QVERIFY(!canvas->isRecordingMacro());
    macroPanel->refresh();

    QCOMPARE(canvas->document().macros().size(), size_t(1));
    const Macro &macro = canvas->document().macros().front();
    QCOMPARE(macro.name, QStringLiteral("Style A"));
    QCOMPARE(macro.steps.size(), 1);
    QCOMPARE(macro.steps.front().kind, MacroStep::Kind::SetShadow);
    QVERIFY(macro.steps.front().enabled);

    auto *list = macroPanel->findChild<QListWidget *>();
    QVERIFY(list);
    QCOMPARE(list->count(), 1);

    // Rejouer sur une autre forme, non enregistrée pendant la capture.
    Shape *other = layer->addShape(std::make_unique<RectShape>(QRectF(100, 100, 20, 20)));
    QVERIFY(!other->shadowEnabled);
    toolbox->actions()[kSelectionIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, canvas->mapFromScene(QPointF(110, 110)));

    canvas->playMacro(macro);
    QVERIFY(other->shadowEnabled);

    canvas->document().undoStack()->undo();
    QVERIFY(!other->shadowEnabled);
}

void UiTests::traceImageFileAddsClosedShapesFilledByPalette() {
    QImage image(8, 8, QImage::Format_RGB32);
    image.fill(Qt::white);
    for (int y = 2; y < 5; ++y) {
        for (int x = 2; x < 5; ++x) {
            image.setPixel(x, y, qRgb(0, 0, 0));
        }
    }

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString path = dir.filePath("trace_source.png");
    QVERIFY(image.save(path));

    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    QVERIFY(canvas);

    Layer *layer = canvas->document().activeLayer();
    QCOMPARE(layer->shapeCount(), size_t(0));

    QString error;
    QVERIFY2(canvas->traceImageFile(path, &error), qPrintable(error));
    QCOMPARE(layer->shapeCount(), size_t(1));

    auto *traced = dynamic_cast<PathShape *>(layer->shapes().front().get());
    QVERIFY(traced);
    QVERIFY(traced->closed);
    const QColor originalStroke = traced->strokeColor;

    // La couleur active doit s'appliquer au remplissage (silhouette), pas au
    // trait, contrairement à un tracé ouvert de l'outil Plume.
    canvas->setActiveColor(QColor(0, 200, 0));
    QCOMPARE(traced->fillColor, QColor(0, 200, 0));
    QCOMPARE(traced->strokeColor, originalStroke);

    canvas->document().undoStack()->undo(); // annule le changement de couleur
    canvas->document().undoStack()->undo(); // annule l'ajout des formes vectorisées
    QCOMPARE(layer->shapeCount(), size_t(0));
}

void UiTests::multiLineTextBoundsTallerThanSingleLine() {
    TextShape singleLine(QPointF(0, 0), QStringLiteral("AG Draw"));
    TextShape threeLines(QPointF(0, 0), QStringLiteral("AG Draw\nest\nun logiciel"));

    QVERIFY(threeLines.bounds().height() > singleLine.bounds().height() * 2);
    // La largeur ne doit pas exploser : chaque ligne est bornée indépendamment.
    QVERIFY(threeLines.bounds().width() < singleLine.bounds().width() * 5);
}

void UiTests::textToolCreatesMultiLineShape() {
    MainWindow window;
    window.show();
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    QVERIFY(canvas && toolbox);
    canvas->setFocus();

    toolbox->actions()[kTextIndex]->trigger();
    QTest::mouseClick(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));

    auto *editor = canvas->findChild<QPlainTextEdit *>();
    QVERIFY(editor);
    QVERIFY(editor->isVisible());

    QTest::keyClicks(editor, QStringLiteral("Ligne 1"));
    QTest::keyClick(editor, Qt::Key_Return, Qt::ShiftModifier); // saut de ligne, ne valide pas
    QTest::keyClicks(editor, QStringLiteral("Ligne 2"));
    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(0)); // pas encore validé

    QTest::keyClick(editor, Qt::Key_Return); // valide (sans Maj)
    QVERIFY(!editor->isVisible());

    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(1));
    auto *text = dynamic_cast<TextShape *>(canvas->document().activeLayer()->shapes().front().get());
    QVERIFY(text);
    QCOMPARE(text->text, QStringLiteral("Ligne 1\nLigne 2"));
}

void UiTests::pagesPanelAddsAndNavigatesPages() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *pagesPanel = window.findChild<PagesPanel *>("PagesPanel");
    QVERIFY(canvas && pagesPanel);

    QCOMPARE(canvas->document().pages().size(), size_t(1));

    auto *toolbar = pagesPanel->findChild<QToolBar *>();
    QVERIFY(toolbar && !toolbar->actions().isEmpty());
    toolbar->actions().first()->trigger(); // "+ Page"

    QCOMPARE(canvas->document().pages().size(), size_t(2));
    // La nouvelle page devient active et la vue y navigue.
    QCOMPARE(canvas->document().activePage(), canvas->document().pages().back().get());

    auto *list = pagesPanel->findChild<QListWidget *>();
    QVERIFY(list);
    QCOMPARE(list->count(), 2);

    list->setCurrentRow(0);
    QCOMPARE(canvas->document().activePage(), canvas->document().pages().front().get());
}

void UiTests::newDocumentResetsToOnePage() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    QVERIFY(canvas);

    canvas->document().addPage(QStringLiteral("Extra"), QRectF(0, 1200, 794, 1123));
    QCOMPARE(canvas->document().pages().size(), size_t(2));

    canvas->newDocument();
    QCOMPARE(canvas->document().pages().size(), size_t(1));
    QCOMPARE(canvas->document().activePage()->name(), QStringLiteral("Page 1"));
}

void UiTests::brushToolCreatesStrokeWithMouse() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    auto *toolbox = window.findChild<ToolBox *>("ToolBox");
    QVERIFY(canvas && toolbox);
    canvas->setFocus();

    toolbox->actions()[kBrushIndex]->trigger();
    QTest::mousePress(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(100, 100));
    QTest::mouseMove(canvas->viewport(), QPoint(150, 120));
    QTest::mouseMove(canvas->viewport(), QPoint(200, 100));
    QTest::mouseRelease(canvas->viewport(), Qt::LeftButton, Qt::NoModifier, QPoint(200, 100));

    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(1));
    auto *brush = dynamic_cast<BrushStroke *>(canvas->document().activeLayer()->shapes().front().get());
    QVERIFY(brush);
    QVERIFY(brush->points.size() >= 3);
    for (const auto &point : brush->points) {
        // À la souris (sans tablette), la pression est simulée constante.
        QCOMPARE(point.pressure, 1.0);
    }

    canvas->document().undoStack()->undo();
    QCOMPARE(canvas->document().activeLayer()->shapeCount(), size_t(0));
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

void UiTests::exportPdfAndSeparationsThroughCanvas() {
    MainWindow window;
    auto *canvas = window.findChild<CanvasView *>();
    QVERIFY(canvas);

    canvas->document().activeLayer()->addShape(std::make_unique<RectShape>(QRectF(0, 0, 40, 40)));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    QString error;
    const QString pdfPath = dir.filePath("export.pdf");
    QVERIFY2(canvas->exportToPdf(pdfPath, /*includeCropMarks=*/true, &error), qPrintable(error));
    QFile pdfFile(pdfPath);
    QVERIFY(pdfFile.exists());
    QVERIFY(pdfFile.size() > 0);

    const QString sepBasePath = dir.filePath("sep");
    QVERIFY2(canvas->exportColorSeparations(sepBasePath, &error), qPrintable(error));
    QVERIFY(QFile::exists(sepBasePath + "_C.png"));
    QVERIFY(QFile::exists(sepBasePath + "_M.png"));
    QVERIFY(QFile::exists(sepBasePath + "_Y.png"));
    QVERIFY(QFile::exists(sepBasePath + "_K.png"));
}

QTEST_MAIN(UiTests)
#include "UiTests.moc"
