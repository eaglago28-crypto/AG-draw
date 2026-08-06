#include "CanvasView.h"

#include "AgdDocumentIO.h"
#include "BitmapTracer.h"
#include "BrushStroke.h"
#include "ColorSeparationExporter.h"
#include "Commands.h"
#include "Document.h"
#include "DocumentItem.h"
#include "EllipseShape.h"
#include "Layer.h"
#include "Page.h"
#include "PdfExporter.h"
#include "PngExporter.h"
#include "PowerClipGroup.h"
#include "RectShape.h"
#include "ShapeRecognizer.h"
#include "TextShape.h"
#include "TextToCurves.h"

#include <algorithm>

#include <QGraphicsScene>
#include <QImage>
#include <QKeyEvent>
#include <QPlainTextEdit>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QScrollBar>
#include <QTabletEvent>
#include <QUndoStack>
#include <QWheelEvent>

namespace agdraw::ui {

namespace {
constexpr qreal kSceneExtent = 100000.0;
constexpr qreal kGridStep = 50.0;
constexpr qreal kMinZoom = 0.05;
constexpr qreal kMaxZoom = 40.0;
constexpr qreal kHandleRadiusPx = 6.0;
constexpr qreal kMinShapeSize = 2.0;
constexpr qreal kMinPenHandleLength = 3.0;

// Les tracés ouverts (outil Plume) et les traits de pinceau n'ont pas de
// remplissage visible : la couleur active agit sur strokeColor. Un tracé
// fermé (silhouette issue de la vectorisation de bitmap) est traité comme
// une forme à surface normale : la couleur active agit sur fillColor.
bool isStrokeOnlyShape(engine::Shape *shape) {
    if (auto *path = dynamic_cast<engine::PathShape *>(shape)) {
        return !path->closed;
    }
    return dynamic_cast<engine::BrushStroke *>(shape) != nullptr;
}
} // namespace

CanvasView::CanvasView(QWidget *parent)
    : QGraphicsView(parent), m_document(std::make_unique<engine::Document>()) {
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setupScene();
    setupTextEditor();
}

CanvasView::~CanvasView() = default;

engine::Document &CanvasView::document() {
    return *m_document;
}

void CanvasView::setupScene() {
    auto *scene = new QGraphicsScene(-kSceneExtent, -kSceneExtent, 2 * kSceneExtent, 2 * kSceneExtent, this);
    setScene(scene);

    // Les pages elles-mêmes (rectangles blancs nommés) sont dessinées par
    // DocumentItem à partir du modèle Document::pages(), pas codées en dur
    // ici : un document peut avoir zéro, une ou plusieurs pages.
    m_documentItem = new DocumentItem(*m_document, QRectF(-kSceneExtent, -kSceneExtent, 2 * kSceneExtent, 2 * kSceneExtent));
    m_documentItem->setZValue(1);
    scene->addItem(m_documentItem);

    if (engine::Page *page = m_document->activePage()) {
        centerOn(page->rect().center());
    }
}

void CanvasView::setupTextEditor() {
    m_textEditor = new QPlainTextEdit(viewport());
    m_textEditor->hide();
    m_textEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_textEditor->installEventFilter(this);
}

void CanvasView::setActiveTool(Tool tool) {
    m_penNodes.clear();
    m_penDraggingHandle = false;
    m_brushPoints.clear();
    m_brushDragging = false;
    m_rubberBanding = false;
    m_resizing = false;
    m_dragging = false;
    if (m_textEditor->isVisible()) {
        cancelTextEditor();
    }
    m_activeTool = tool;
    setSelection({});
    m_documentItem->update();
}

void CanvasView::setActiveColor(const QColor &color) {
    m_currentColor = color;
    m_colorExplicitlySet = true;
    if (!m_selection.isEmpty()) {
        m_document->undoStack()->beginMacro(tr("Couleur"));
        for (engine::Shape *shape : m_selection) {
            if (isStrokeOnlyShape(shape)) {
                m_document->undoStack()->push(new engine::SetStrokeColorCommand(shape, shape->strokeColor, color));
            } else {
                m_document->undoStack()->push(new engine::SetFillColorCommand(shape, shape->fillColor, color));
            }
        }
        m_document->undoStack()->endMacro();
        if (m_recordingMacro) {
            engine::MacroStep step;
            step.kind = engine::MacroStep::Kind::SetFillColor;
            step.color = color;
            recordMacroStep(step);
        }
        m_documentItem->update();
    }
    emit statusMessage(tr("Couleur active : %1").arg(color.name()));
}

void CanvasView::applyCurrentColor(engine::Shape *shape) const {
    if (isStrokeOnlyShape(shape)) {
        shape->strokeColor = m_currentColor;
    } else {
        shape->fillColor = m_currentColor;
    }
}

void CanvasView::setSelection(QVector<engine::Shape *> newSelection) {
    m_selection = std::move(newSelection);
    emit selectionChanged(m_selection.size() == 1 ? m_selection.first() : nullptr);
    emit selectionCountChanged(m_selection.size());
}

void CanvasView::reorderSelection(bool forward, bool toExtreme) {
    if (m_selection.size() != 1) {
        return;
    }
    engine::Shape *shape = m_selection.first();
    engine::Layer *owner = m_document->findLayerOf(shape);
    if (!owner) {
        return;
    }

    const size_t from = owner->indexOf(shape);
    const size_t count = owner->shapeCount();
    if (count == 0) {
        return;
    }

    size_t to = from;
    QString label;
    if (toExtreme) {
        to = forward ? count - 1 : 0;
        label = forward ? tr("Premier plan") : tr("Arrière-plan");
    } else {
        to = forward ? (from + 1 < count ? from + 1 : count - 1) : (from == 0 ? 0 : from - 1);
        label = forward ? tr("Avancer") : tr("Reculer");
    }

    if (to != from) {
        m_document->undoStack()->push(new engine::ReorderShapeCommand(owner, from, to, label));
        m_documentItem->update();
        emit statusMessage(label);
    }
}

void CanvasView::newDocument() {
    m_document->clearLayers();
    m_document->addLayer(tr("Calque 1"));
    m_document->clearPages();
    m_document->addPage(tr("Page 1"), QRectF(0, 0, 794, 1123));
    m_document->clearMacros();
    m_document->undoStack()->clear();
    setSelection({});
    m_penNodes.clear();
    m_penDraggingHandle = false;
    m_documentItem->update();
    if (engine::Page *page = m_document->activePage()) {
        centerOn(page->rect().center());
    }
    emit statusMessage(tr("Nouveau document"));
}

bool CanvasView::saveToFile(const QString &path, QString *errorMessage) {
    return io::saveAgd(*m_document, path, errorMessage);
}

bool CanvasView::loadFromFile(const QString &path, QString *errorMessage) {
    if (!io::loadAgd(*m_document, path, errorMessage)) {
        return false;
    }
    setSelection({});
    m_penNodes.clear();
    m_penDraggingHandle = false;
    m_documentItem->update();
    emit statusMessage(tr("Document chargé"));
    return true;
}

bool CanvasView::exportToPng(const QString &path, QString *errorMessage) {
    engine::Page *page = m_document->activePage();
    if (!page) {
        if (errorMessage) {
            *errorMessage = tr("Aucune page à exporter.");
        }
        return false;
    }
    const QRectF pageRect = page->rect();
    return io::exportPng(*m_document, path, pageRect, pageRect.size().toSize(), errorMessage);
}

bool CanvasView::exportToPdf(const QString &path, bool includeCropMarks, QString *errorMessage) {
    engine::Page *page = m_document->activePage();
    if (!page) {
        if (errorMessage) {
            *errorMessage = tr("Aucune page à exporter.");
        }
        return false;
    }
    return io::exportPdf(*m_document, path, page->rect(), includeCropMarks, errorMessage);
}

bool CanvasView::exportColorSeparations(const QString &basePath, QString *errorMessage) {
    engine::Page *page = m_document->activePage();
    if (!page) {
        if (errorMessage) {
            *errorMessage = tr("Aucune page à exporter.");
        }
        return false;
    }
    const QRectF pageRect = page->rect();
    return io::exportColorSeparations(*m_document, basePath, pageRect, pageRect.size().toSize(), errorMessage);
}

bool CanvasView::traceImageFile(const QString &path, QString *errorMessage) {
    QImage image(path);
    if (image.isNull()) {
        if (errorMessage) {
            *errorMessage = tr("Impossible de lire cette image.");
        }
        return false;
    }

    engine::Page *page = m_document->activePage();
    const QRectF targetRect = page ? page->rect() : QRectF(0, 0, 400, 400);

    auto shapes = engine::traceBitmap(image, targetRect);
    if (shapes.empty()) {
        if (errorMessage) {
            *errorMessage = tr("Aucune forme détectée dans cette image (essayez une image plus contrastée).");
        }
        return false;
    }

    engine::Layer *layer = m_document->activeLayer();
    m_document->undoStack()->beginMacro(tr("Vectoriser une image"));
    QVector<engine::Shape *> traced;
    for (auto &shape : shapes) {
        auto *command = new engine::AddShapeCommand(layer, std::move(shape), tr("Vectoriser une image"));
        m_document->undoStack()->push(command);
        traced.append(command->shapePtr());
    }
    m_document->undoStack()->endMacro();
    setSelection(traced);
    m_documentItem->update();
    emit statusMessage(tr("%1 forme(s) vectorisée(s)").arg(traced.size()));
    return true;
}

void CanvasView::goToPage(engine::Page *page) {
    if (!page) {
        return;
    }
    m_document->setActivePage(page);
    centerOn(page->rect().center());
    emit statusMessage(tr("Page : %1").arg(page->name()));
}

void CanvasView::setSelectionShadow(bool enabled) {
    QVector<engine::Shape *> targets;
    for (engine::Shape *shape : m_selection) {
        if (shape->supportsFillEffects()) {
            targets.append(shape);
        }
    }
    if (targets.isEmpty()) {
        return;
    }
    if (targets.size() == 1) {
        m_document->undoStack()->push(new engine::SetShadowCommand(targets.first(), targets.first()->shadowEnabled, enabled));
    } else {
        m_document->undoStack()->beginMacro(tr("Ombre portée"));
        for (engine::Shape *shape : targets) {
            m_document->undoStack()->push(new engine::SetShadowCommand(shape, shape->shadowEnabled, enabled));
        }
        m_document->undoStack()->endMacro();
    }
    if (m_recordingMacro) {
        engine::MacroStep step;
        step.kind = engine::MacroStep::Kind::SetShadow;
        step.enabled = enabled;
        recordMacroStep(step);
    }
    m_documentItem->update();
}

void CanvasView::setSelectionGradient(bool enabled) {
    QVector<engine::Shape *> targets;
    for (engine::Shape *shape : m_selection) {
        if (shape->supportsFillEffects()) {
            targets.append(shape);
        }
    }
    if (targets.isEmpty()) {
        return;
    }
    if (targets.size() == 1) {
        m_document->undoStack()->push(new engine::SetGradientCommand(targets.first(), targets.first()->gradientEnabled, enabled));
    } else {
        m_document->undoStack()->beginMacro(tr("Dégradé"));
        for (engine::Shape *shape : targets) {
            m_document->undoStack()->push(new engine::SetGradientCommand(shape, shape->gradientEnabled, enabled));
        }
        m_document->undoStack()->endMacro();
    }
    if (m_recordingMacro) {
        engine::MacroStep step;
        step.kind = engine::MacroStep::Kind::SetGradient;
        step.enabled = enabled;
        recordMacroStep(step);
    }
    m_documentItem->update();
}

void CanvasView::setSelectionContour(bool enabled) {
    QVector<engine::Shape *> targets;
    for (engine::Shape *shape : m_selection) {
        if (shape->supportsFillEffects()) {
            targets.append(shape);
        }
    }
    if (targets.isEmpty()) {
        return;
    }
    if (targets.size() == 1) {
        m_document->undoStack()->push(new engine::SetContourCommand(targets.first(), targets.first()->contourEnabled, enabled));
    } else {
        m_document->undoStack()->beginMacro(tr("Contour"));
        for (engine::Shape *shape : targets) {
            m_document->undoStack()->push(new engine::SetContourCommand(shape, shape->contourEnabled, enabled));
        }
        m_document->undoStack()->endMacro();
    }
    if (m_recordingMacro) {
        engine::MacroStep step;
        step.kind = engine::MacroStep::Kind::SetContour;
        step.enabled = enabled;
        recordMacroStep(step);
    }
    m_documentItem->update();
}

void CanvasView::setSelectionEnvelope(bool enabled) {
    QVector<engine::Shape *> targets;
    for (engine::Shape *shape : m_selection) {
        if (shape->supportsFillEffects()) {
            targets.append(shape);
        }
    }
    if (targets.isEmpty()) {
        return;
    }
    if (targets.size() == 1) {
        engine::Shape *shape = targets.first();
        const QVector<QPointF> newCorners =
            enabled && shape->envelopeCorners.size() != 4 ? engine::defaultEnvelopeCorners(shape->bounds()) : shape->envelopeCorners;
        m_document->undoStack()->push(
            new engine::SetEnvelopeCommand(shape, shape->envelopeEnabled, enabled, shape->envelopeCorners, newCorners));
    } else {
        m_document->undoStack()->beginMacro(tr("Enveloppe"));
        for (engine::Shape *shape : targets) {
            const QVector<QPointF> newCorners = enabled && shape->envelopeCorners.size() != 4
                                                     ? engine::defaultEnvelopeCorners(shape->bounds())
                                                     : shape->envelopeCorners;
            m_document->undoStack()->push(new engine::SetEnvelopeCommand(shape, shape->envelopeEnabled, enabled,
                                                                          shape->envelopeCorners, newCorners));
        }
        m_document->undoStack()->endMacro();
    }
    m_documentItem->update();
}

void CanvasView::setSelectionExtrusion(bool enabled) {
    QVector<engine::Shape *> targets;
    for (engine::Shape *shape : m_selection) {
        if (shape->supportsFillEffects()) {
            targets.append(shape);
        }
    }
    if (targets.isEmpty()) {
        return;
    }
    if (targets.size() == 1) {
        m_document->undoStack()->push(
            new engine::SetExtrusionCommand(targets.first(), targets.first()->extrusionEnabled, enabled));
    } else {
        m_document->undoStack()->beginMacro(tr("Extrusion"));
        for (engine::Shape *shape : targets) {
            m_document->undoStack()->push(new engine::SetExtrusionCommand(shape, shape->extrusionEnabled, enabled));
        }
        m_document->undoStack()->endMacro();
    }
    if (m_recordingMacro) {
        engine::MacroStep step;
        step.kind = engine::MacroStep::Kind::SetExtrusion;
        step.enabled = enabled;
        recordMacroStep(step);
    }
    m_documentItem->update();
}

void CanvasView::alignSelection(AlignMode mode) {
    if (m_selection.size() < 2) {
        return;
    }

    QRectF unionBounds = m_selection.first()->bounds();
    for (engine::Shape *shape : m_selection) {
        unionBounds = unionBounds.united(shape->bounds());
    }

    QVector<QPointF> deltas;
    deltas.reserve(m_selection.size());
    for (engine::Shape *shape : m_selection) {
        const QRectF bounds = shape->bounds();
        QPointF delta(0, 0);
        switch (mode) {
            case AlignMode::Left: delta.setX(unionBounds.left() - bounds.left()); break;
            case AlignMode::HCenter: delta.setX(unionBounds.center().x() - bounds.center().x()); break;
            case AlignMode::Right: delta.setX(unionBounds.right() - bounds.right()); break;
            case AlignMode::Top: delta.setY(unionBounds.top() - bounds.top()); break;
            case AlignMode::VCenter: delta.setY(unionBounds.center().y() - bounds.center().y()); break;
            case AlignMode::Bottom: delta.setY(unionBounds.bottom() - bounds.bottom()); break;
        }
        deltas.append(delta);
    }

    m_document->undoStack()->beginMacro(tr("Aligner"));
    for (int i = 0; i < m_selection.size(); ++i) {
        if (!deltas[i].isNull()) {
            m_document->undoStack()->push(new engine::TranslateShapeCommand(m_selection[i], deltas[i]));
        }
    }
    m_document->undoStack()->endMacro();
    m_documentItem->update();
    emit statusMessage(tr("Aligné"));
}

void CanvasView::distributeSelection(DistributeMode mode) {
    if (m_selection.size() < 3) {
        return;
    }

    QVector<engine::Shape *> sorted = m_selection;
    if (mode == DistributeMode::Horizontal) {
        std::sort(sorted.begin(), sorted.end(), [](engine::Shape *a, engine::Shape *b) {
            return a->bounds().center().x() < b->bounds().center().x();
        });
    } else {
        std::sort(sorted.begin(), sorted.end(), [](engine::Shape *a, engine::Shape *b) {
            return a->bounds().center().y() < b->bounds().center().y();
        });
    }

    const int count = sorted.size();
    const qreal firstCenter = mode == DistributeMode::Horizontal ? sorted.first()->bounds().center().x()
                                                                   : sorted.first()->bounds().center().y();
    const qreal lastCenter =
        mode == DistributeMode::Horizontal ? sorted.last()->bounds().center().x() : sorted.last()->bounds().center().y();
    const qreal step = (lastCenter - firstCenter) / (count - 1);

    m_document->undoStack()->beginMacro(tr("Distribuer"));
    for (int i = 1; i < count - 1; ++i) {
        const qreal target = firstCenter + step * i;
        const qreal current =
            mode == DistributeMode::Horizontal ? sorted[i]->bounds().center().x() : sorted[i]->bounds().center().y();
        const qreal offset = target - current;
        if (offset != 0.0) {
            const QPointF delta = mode == DistributeMode::Horizontal ? QPointF(offset, 0) : QPointF(0, offset);
            m_document->undoStack()->push(new engine::TranslateShapeCommand(sorted[i], delta));
        }
    }
    m_document->undoStack()->endMacro();
    m_documentItem->update();
    emit statusMessage(tr("Distribué"));
}

void CanvasView::blendSelection() {
    if (m_selection.size() != 2) {
        emit statusMessage(tr("Sélectionnez exactement deux formes pour créer un fondu"));
        return;
    }

    engine::Shape *shapeA = m_selection.first();
    engine::Shape *shapeB = m_selection.last();

    const bool bothRects = dynamic_cast<engine::RectShape *>(shapeA) && dynamic_cast<engine::RectShape *>(shapeB);
    const bool bothEllipses =
        dynamic_cast<engine::EllipseShape *>(shapeA) && dynamic_cast<engine::EllipseShape *>(shapeB);
    if (!bothRects && !bothEllipses) {
        emit statusMessage(tr("Le fondu nécessite deux rectangles ou deux ellipses"));
        return;
    }

    engine::Layer *layer = m_document->findLayerOf(shapeA);
    if (!layer) {
        return;
    }

    constexpr int kBlendSteps = 5;
    const QRectF boundsA = shapeA->bounds();
    const QRectF boundsB = shapeB->bounds();

    m_document->undoStack()->beginMacro(tr("Fondu"));
    for (int i = 1; i <= kBlendSteps; ++i) {
        const qreal t = static_cast<qreal>(i) / (kBlendSteps + 1);
        const QRectF interpolated(boundsA.x() + (boundsB.x() - boundsA.x()) * t,
                                   boundsA.y() + (boundsB.y() - boundsA.y()) * t,
                                   boundsA.width() + (boundsB.width() - boundsA.width()) * t,
                                   boundsA.height() + (boundsB.height() - boundsA.height()) * t);

        std::unique_ptr<engine::Shape> intermediate;
        if (bothRects) {
            intermediate = std::make_unique<engine::RectShape>(interpolated);
        } else {
            intermediate = std::make_unique<engine::EllipseShape>(interpolated);
        }
        intermediate->fillColor = engine::interpolateColor(shapeA->fillColor, shapeB->fillColor, t);
        intermediate->strokeColor = engine::interpolateColor(shapeA->strokeColor, shapeB->strokeColor, t);
        intermediate->strokeWidth = shapeA->strokeWidth + (shapeB->strokeWidth - shapeA->strokeWidth) * t;

        m_document->undoStack()->push(new engine::AddShapeCommand(layer, std::move(intermediate), tr("Fondu")));
    }
    m_document->undoStack()->endMacro();
    m_documentItem->update();
    emit statusMessage(tr("Fondu créé"));
}

void CanvasView::applyPowerClip() {
    if (m_selection.size() < 2) {
        emit statusMessage(tr("Sélectionnez au moins deux formes pour créer un PowerClip"));
        return;
    }

    engine::Shape *container = m_selection.last();
    if (!container->supportsFillEffects()) {
        emit statusMessage(tr("Le contenant du PowerClip doit être un rectangle ou une ellipse (dernière forme "
                               "sélectionnée)"));
        return;
    }

    engine::Layer *layer = m_document->findLayerOf(container);
    if (!layer) {
        return;
    }
    for (engine::Shape *shape : m_selection) {
        if (m_document->findLayerOf(shape) != layer) {
            emit statusMessage(tr("Toutes les formes doivent appartenir au même calque"));
            return;
        }
    }

    auto *command = new engine::ApplyPowerClipCommand(layer, m_selection, container, tr("PowerClip"));
    m_document->undoStack()->push(command);
    setSelection({command->groupPtr()});
    m_documentItem->update();
    emit statusMessage(tr("PowerClip créé"));
}

void CanvasView::convertSelectionToCurves() {
    QVector<engine::TextShape *> targets;
    for (engine::Shape *shape : m_selection) {
        if (auto *text = dynamic_cast<engine::TextShape *>(shape)) {
            targets.append(text);
        }
    }
    if (targets.isEmpty()) {
        emit statusMessage(tr("Sélectionnez du texte pour le convertir en courbes"));
        return;
    }

    QVector<engine::Shape *> newShapes;
    m_document->undoStack()->beginMacro(tr("Convertir en courbes"));
    for (engine::TextShape *text : targets) {
        engine::Layer *layer = m_document->findLayerOf(text);
        if (!layer) {
            continue;
        }
        std::vector<std::unique_ptr<engine::Shape>> curves = engine::textToCurves(*text);
        for (auto &curve : curves) {
            auto *command = new engine::AddShapeCommand(layer, std::move(curve), tr("Convertir en courbes"));
            m_document->undoStack()->push(command);
            newShapes.append(command->shapePtr());
        }
        m_document->undoStack()->push(new engine::RemoveShapeCommand(layer, text, tr("Convertir en courbes")));
    }
    m_document->undoStack()->endMacro();

    setSelection(newShapes);
    m_documentItem->update();
    emit statusMessage(tr("%1 forme(s) convertie(s) en courbes").arg(newShapes.size()));
}

void CanvasView::recognizeSelectionShape() {
    QVector<engine::BrushStroke *> targets;
    for (engine::Shape *shape : m_selection) {
        if (auto *brush = dynamic_cast<engine::BrushStroke *>(shape)) {
            targets.append(brush);
        }
    }
    if (targets.isEmpty()) {
        emit statusMessage(tr("Sélectionnez un tracé au pinceau pour le reconnaître comme une forme"));
        return;
    }

    // Résout la reconnaissance de chaque tracé AVANT de toucher à la pile
    // d'annulation : si aucun n'est reconnu, on ne pousse aucune commande
    // (une macro vide laisserait une entrée fantôme dans l'historique).
    struct PendingReplacement {
        engine::BrushStroke *original;
        engine::Layer *layer;
        std::unique_ptr<engine::Shape> replacement;
    };
    std::vector<PendingReplacement> pending;
    for (engine::BrushStroke *brush : targets) {
        engine::Layer *layer = m_document->findLayerOf(brush);
        if (!layer) {
            continue;
        }
        QVector<QPointF> points;
        points.reserve(brush->points.size());
        for (const engine::BrushPoint &point : brush->points) {
            points.append(point.point);
        }
        engine::ShapeRecognitionResult result = engine::recognizeShape(points);
        if (!result.shape) {
            continue;
        }
        result.shape->fillColor = brush->fillColor;
        result.shape->strokeColor = brush->strokeColor;
        result.shape->strokeWidth = brush->strokeWidth;
        pending.push_back({brush, layer, std::move(result.shape)});
    }

    if (pending.empty()) {
        emit statusMessage(tr("Aucune forme reconnaissable (le tracé doit former une boucle fermée)"));
        return;
    }

    QVector<engine::Shape *> newShapes;
    m_document->undoStack()->beginMacro(tr("Reconnaître la forme"));
    for (PendingReplacement &item : pending) {
        auto *addCommand =
            new engine::AddShapeCommand(item.layer, std::move(item.replacement), tr("Reconnaître la forme"));
        m_document->undoStack()->push(addCommand);
        newShapes.append(addCommand->shapePtr());
        m_document->undoStack()->push(new engine::RemoveShapeCommand(item.layer, item.original, tr("Reconnaître la forme")));
    }
    m_document->undoStack()->endMacro();

    setSelection(newShapes);
    m_documentItem->update();
    emit statusMessage(tr("%1 forme(s) reconnue(s)").arg(pending.size()));
}

void CanvasView::startMacroRecording() {
    m_recordingMacro = true;
    m_recordingSteps.clear();
    emit statusMessage(tr("Enregistrement de macro en cours…"));
}

void CanvasView::stopMacroRecording(const QString &name) {
    m_recordingMacro = false;
    if (m_recordingSteps.isEmpty() || name.trimmed().isEmpty()) {
        m_recordingSteps.clear();
        emit statusMessage(tr("Enregistrement annulé (aucune action capturée)"));
        return;
    }
    engine::Macro macro;
    macro.name = name.trimmed();
    macro.steps = m_recordingSteps;
    m_recordingSteps.clear();
    m_document->addMacro(std::move(macro));
    emit statusMessage(tr("Macro « %1 » enregistrée (%2 étape(s))").arg(name.trimmed()).arg(macro.steps.size()));
}

void CanvasView::recordMacroStep(const engine::MacroStep &step) {
    m_recordingSteps.append(step);
}

void CanvasView::playMacro(const engine::Macro &macro) {
    if (m_selection.isEmpty()) {
        emit statusMessage(tr("Sélectionnez au moins une forme pour rejouer une macro"));
        return;
    }
    engine::applyMacro(*m_document, macro, m_selection);
    m_documentItem->update();
    emit statusMessage(tr("Macro « %1 » rejouée").arg(macro.name));
}

void CanvasView::refreshView() {
    m_documentItem->update();
}

bool CanvasView::isEmpty() const {
    for (const auto &layer : m_document->layers()) {
        if (!layer->shapes().empty()) {
            return false;
        }
    }
    return true;
}

void CanvasView::setSelectionStrokeWidth(double width) {
    if (m_selection.isEmpty()) {
        return;
    }
    if (m_selection.size() == 1) {
        engine::Shape *shape = m_selection.first();
        if (shape->strokeWidth != width) {
            m_document->undoStack()->push(new engine::SetStrokeWidthCommand(shape, shape->strokeWidth, width));
        }
    } else {
        m_document->undoStack()->beginMacro(tr("Épaisseur de trait"));
        for (engine::Shape *shape : m_selection) {
            m_document->undoStack()->push(new engine::SetStrokeWidthCommand(shape, shape->strokeWidth, width));
        }
        m_document->undoStack()->endMacro();
    }
    if (m_recordingMacro) {
        engine::MacroStep step;
        step.kind = engine::MacroStep::Kind::SetStrokeWidth;
        step.value = width;
        recordMacroStep(step);
    }
    m_documentItem->update();
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

void CanvasView::drawForeground(QPainter *painter, const QRectF &) {
    const QColor accent(0, 122, 255);

    if (m_creatingShape) {
        QPen previewPen(accent);
        previewPen.setStyle(Qt::DashLine);
        previewPen.setWidth(0);
        painter->setPen(previewPen);
        painter->setBrush(QColor(accent.red(), accent.green(), accent.blue(), 40));
        if (m_activeTool == Tool::Rectangle) {
            painter->drawRect(QRectF(m_dragStart, m_dragCurrent).normalized());
        } else if (m_activeTool == Tool::Ellipse) {
            painter->drawEllipse(QRectF(m_dragStart, m_dragCurrent).normalized());
        }
    }

    if (m_activeTool == Tool::Selection) {
        for (engine::Shape *shape : m_selection) {
            const bool isPrimaryResizing = m_resizing && m_selection.size() == 1 && shape == m_selection.first();
            const QRectF bounds = isPrimaryResizing ? m_pendingBounds : shape->bounds();
            QPen handlePen(accent);
            handlePen.setStyle(Qt::DashLine);
            handlePen.setWidth(0);
            painter->setPen(handlePen);
            painter->setBrush(Qt::NoBrush);
            painter->drawRect(bounds.adjusted(-2, -2, 2, 2));
        }

        if (m_selection.size() == 1 && m_selection.first()->envelopeEnabled &&
            m_selection.first()->envelopeCorners.size() == 4) {
            const qreal handleSize = kHandleRadiusPx / std::max(m_zoom, 0.01);
            painter->setBrush(accent);
            painter->setPen(QPen(Qt::white, 0));
            for (const QPointF &corner : m_selection.first()->envelopeCorners) {
                QPolygonF diamond;
                diamond << corner + QPointF(0, -handleSize) << corner + QPointF(handleSize, 0)
                        << corner + QPointF(0, handleSize) << corner + QPointF(-handleSize, 0);
                painter->drawPolygon(diamond);
            }
        } else if (m_selection.size() == 1 && m_selection.first()->isResizable()) {
            const QRectF bounds = m_resizing ? m_pendingBounds : m_selection.first()->bounds();
            const qreal handleSize = kHandleRadiusPx / std::max(m_zoom, 0.01);
            painter->setBrush(Qt::white);
            painter->setPen(QPen(accent, 0));
            for (const QPointF &corner : {bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()}) {
                painter->drawRect(QRectF(corner - QPointF(handleSize, handleSize) / 2, QSizeF(handleSize, handleSize)));
            }
        }

        if (m_rubberBanding) {
            QPen bandPen(accent);
            bandPen.setStyle(Qt::DashLine);
            bandPen.setWidth(0);
            painter->setPen(bandPen);
            painter->setBrush(QColor(accent.red(), accent.green(), accent.blue(), 30));
            painter->drawRect(m_rubberBandRect);
        }
    }

    if (!m_penNodes.isEmpty()) {
        QPen previewPen(accent);
        previewPen.setStyle(Qt::DashLine);
        previewPen.setWidth(0);
        painter->setPen(previewPen);
        painter->setBrush(Qt::NoBrush);

        QPainterPath preview;
        preview.moveTo(m_penNodes.first().point);
        for (int i = 1; i < m_penNodes.size(); ++i) {
            const engine::PathNode &prev = m_penNodes[i - 1];
            const engine::PathNode &cur = m_penNodes[i];
            if (prev.handle.isNull() && cur.handle.isNull()) {
                preview.lineTo(cur.point);
            } else {
                preview.cubicTo(prev.point + prev.handle, cur.point - cur.handle, cur.point);
            }
        }
        painter->drawPath(preview);

        painter->setBrush(accent);
        for (const engine::PathNode &node : m_penNodes) {
            painter->drawEllipse(node.point, 3, 3);
            if (!node.handle.isNull()) {
                painter->drawLine(node.point - node.handle, node.point + node.handle);
                painter->drawRect(QRectF(node.point + node.handle - QPointF(2, 2), QSizeF(4, 4)));
                painter->drawRect(QRectF(node.point - node.handle - QPointF(2, 2), QSizeF(4, 4)));
            }
        }
    }

    if (m_brushDragging && m_brushPoints.size() >= 2) {
        painter->setPen(Qt::NoPen);
        QColor previewColor = m_currentColor;
        previewColor.setAlpha(180);
        painter->setBrush(previewColor);
        painter->drawPath(engine::buildBrushOutline(m_brushPoints, 8.0));
    }
}

QRectF CanvasView::computeResizedBounds(const QPointF &scenePos) const {
    QRectF newBounds = m_originalBounds;
    switch (m_activeHandle) {
        case 0: newBounds.setTopLeft(scenePos); break;
        case 1: newBounds.setTopRight(scenePos); break;
        case 2: newBounds.setBottomLeft(scenePos); break;
        case 3: newBounds.setBottomRight(scenePos); break;
        default: break;
    }
    return newBounds.normalized();
}

int CanvasView::hitTestHandle(const QPoint &viewPos) const {
    if (m_selection.size() != 1 || !m_selection.first()->isResizable() || m_selection.first()->envelopeEnabled) {
        return -1;
    }
    const QRectF bounds = m_selection.first()->bounds();
    const QPointF corners[4] = {bounds.topLeft(), bounds.topRight(), bounds.bottomLeft(), bounds.bottomRight()};
    for (int i = 0; i < 4; ++i) {
        const QPoint handlePos = mapFromScene(corners[i]);
        if ((handlePos - viewPos).manhattanLength() <= kHandleRadiusPx * 2) {
            return i;
        }
    }
    return -1;
}

int CanvasView::hitTestEnvelopeHandle(const QPoint &viewPos) const {
    if (m_selection.size() != 1) {
        return -1;
    }
    engine::Shape *shape = m_selection.first();
    if (!shape->envelopeEnabled || shape->envelopeCorners.size() != 4) {
        return -1;
    }
    for (int i = 0; i < 4; ++i) {
        const QPoint handlePos = mapFromScene(shape->envelopeCorners[i]);
        if ((handlePos - viewPos).manhattanLength() <= kHandleRadiusPx * 2) {
            return i;
        }
    }
    return -1;
}

QVector<engine::Shape *> CanvasView::shapesInRect(const QRectF &rect) const {
    QVector<engine::Shape *> result;
    for (const auto &layer : m_document->layers()) {
        if (!layer->isVisible() || layer->isLocked()) {
            continue;
        }
        for (const auto &shape : layer->shapes()) {
            if (rect.intersects(shape->bounds())) {
                result.append(shape.get());
            }
        }
    }
    return result;
}

void CanvasView::mousePressEvent(QMouseEvent *event) {
    if (m_textEditor->isVisible()) {
        commitTextEditor();
    }

    if (event->button() == Qt::MiddleButton) {
        m_panning = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    if (event->button() != Qt::LeftButton) {
        QGraphicsView::mousePressEvent(event);
        return;
    }

    const QPointF scenePos = mapToScene(event->pos());

    switch (m_activeTool) {
        case Tool::Selection: {
            const int envelopeHandle = hitTestEnvelopeHandle(event->pos());
            if (envelopeHandle >= 0) {
                m_envelopeDragging = true;
                m_envelopeHandle = envelopeHandle;
                m_originalEnvelopeCorners = m_selection.first()->envelopeCorners;
                break;
            }

            const int handle = hitTestHandle(event->pos());
            if (handle >= 0) {
                m_resizing = true;
                m_activeHandle = handle;
                m_originalBounds = m_selection.first()->bounds();
                m_pendingBounds = m_originalBounds;
                break;
            }

            engine::Shape *hitShape = m_document->shapeAt(scenePos);
            const bool shiftHeld = event->modifiers() & Qt::ShiftModifier;
            if (hitShape) {
                if (shiftHeld) {
                    QVector<engine::Shape *> newSelection = m_selection;
                    if (newSelection.contains(hitShape)) {
                        newSelection.removeAll(hitShape);
                    } else {
                        newSelection.append(hitShape);
                    }
                    setSelection(newSelection);
                } else if (!m_selection.contains(hitShape)) {
                    setSelection({hitShape});
                }
                m_dragging = !m_selection.isEmpty();
                m_dragStart = scenePos;
                m_lastMovePos = scenePos;
                emit statusMessage(tr("%1 forme(s) sélectionnée(s)").arg(m_selection.size()));
            } else {
                if (!shiftHeld) {
                    setSelection({});
                }
                m_rubberBanding = true;
                m_rubberBandStart = scenePos;
                m_rubberBandRect = QRectF(scenePos, scenePos);
                emit statusMessage(tr("Aucune forme sous le curseur"));
            }
            break;
        }
        case Tool::Rectangle:
        case Tool::Ellipse:
            m_dragStart = scenePos;
            m_dragCurrent = scenePos;
            m_creatingShape = true;
            break;
        case Tool::Pen:
            m_penNodes.append(engine::PathNode{scenePos, QPointF(0, 0)});
            m_penDraggingHandle = true;
            emit statusMessage(tr("Plume : %1 point(s) — glisser pour une courbe, double-cliquez pour terminer, "
                                   "Échap pour annuler")
                                    .arg(m_penNodes.size()));
            break;
        case Tool::Text: {
            m_textEditorScenePos = scenePos;
            const QPoint viewPos = event->pos();
            m_textEditor->setGeometry(viewPos.x(), viewPos.y(), 260, 90);
            m_textEditor->clear();
            m_textEditor->show();
            m_textEditor->setFocus();
            emit statusMessage(tr("Texte : Entrée valide, Maj+Entrée pour une nouvelle ligne, Échap annule"));
            break;
        }
        case Tool::Brush:
            m_brushPoints.clear();
            m_brushPoints.append(engine::BrushPoint{scenePos, 1.0});
            m_brushDragging = true;
            emit statusMessage(tr("Pinceau : glissez pour tracer (souris = pression constante)"));
            break;
    }

    m_documentItem->update();
    event->accept();
}

void CanvasView::mouseMoveEvent(QMouseEvent *event) {
    if (m_panning) {
        const QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    const QPointF scenePos = mapToScene(event->pos());

    if (m_envelopeDragging) {
        engine::Shape *shape = m_selection.first();
        QVector<QPointF> corners = shape->envelopeCorners;
        corners[m_envelopeHandle] = scenePos;
        shape->envelopeCorners = corners;
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_resizing) {
        m_pendingBounds = computeResizedBounds(scenePos);
        m_selection.first()->setBounds(m_pendingBounds);
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_creatingShape) {
        m_dragCurrent = scenePos;
        viewport()->update();
        event->accept();
        return;
    }

    if (m_rubberBanding) {
        m_rubberBandRect = QRectF(m_rubberBandStart, scenePos).normalized();
        viewport()->update();
        event->accept();
        return;
    }

    if (m_activeTool == Tool::Pen && m_penDraggingHandle && !m_penNodes.isEmpty()) {
        m_penNodes.last().handle = scenePos - m_penNodes.last().point;
        viewport()->update();
        event->accept();
        return;
    }

    if (m_activeTool == Tool::Brush && m_brushDragging) {
        m_brushPoints.append(engine::BrushPoint{scenePos, 1.0});
        viewport()->update();
        event->accept();
        return;
    }

    if (!m_dragging) {
        QGraphicsView::mouseMoveEvent(event);
        return;
    }

    if (m_activeTool == Tool::Selection && !m_selection.isEmpty()) {
        const QPointF delta = scenePos - m_lastMovePos;
        for (engine::Shape *shape : m_selection) {
            shape->translate(delta);
        }
        m_lastMovePos = scenePos;
    }

    m_documentItem->update();
    event->accept();
}

void CanvasView::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton && m_panning) {
        m_panning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }

    if (m_envelopeDragging) {
        m_envelopeDragging = false;
        engine::Shape *shape = m_selection.first();
        QVector<QPointF> finalCorners = shape->envelopeCorners;
        finalCorners[m_envelopeHandle] = mapToScene(event->pos());
        shape->envelopeCorners = m_originalEnvelopeCorners;
        if (finalCorners != m_originalEnvelopeCorners) {
            m_document->undoStack()->push(new engine::SetEnvelopeCornersCommand(shape, m_originalEnvelopeCorners, finalCorners));
        }
        emit statusMessage(tr("Prêt"));
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_resizing) {
        m_resizing = false;
        const QRectF finalBounds = computeResizedBounds(mapToScene(event->pos()));
        engine::Shape *shape = m_selection.first();
        shape->setBounds(m_originalBounds);
        if (finalBounds != m_originalBounds) {
            m_document->undoStack()->push(new engine::ResizeShapeCommand(shape, m_originalBounds, finalBounds));
        }
        emit statusMessage(tr("Prêt"));
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_creatingShape) {
        m_creatingShape = false;
        const QRectF rect = QRectF(m_dragStart, mapToScene(event->pos())).normalized();
        if (rect.width() > kMinShapeSize && rect.height() > kMinShapeSize) {
            std::unique_ptr<engine::Shape> shape;
            QString label;
            if (m_activeTool == Tool::Rectangle) {
                shape = std::make_unique<engine::RectShape>(rect);
                label = tr("Rectangle");
            } else {
                shape = std::make_unique<engine::EllipseShape>(rect);
                label = tr("Ellipse");
            }
            applyCurrentColor(shape.get());
            auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), label);
            m_document->undoStack()->push(command);
            setSelection({command->shapePtr()});
        }
        m_documentItem->update();
        viewport()->update();
        event->accept();
        return;
    }

    if (m_rubberBanding) {
        m_rubberBanding = false;
        const QVector<engine::Shape *> found = shapesInRect(m_rubberBandRect);
        QVector<engine::Shape *> newSelection = m_selection;
        for (engine::Shape *shape : found) {
            if (!newSelection.contains(shape)) {
                newSelection.append(shape);
            }
        }
        setSelection(newSelection);
        m_rubberBandRect = QRectF();
        emit statusMessage(m_selection.isEmpty() ? tr("Aucune forme sous le curseur")
                                                   : tr("%1 forme(s) sélectionnée(s)").arg(m_selection.size()));
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_activeTool == Tool::Pen && m_penDraggingHandle) {
        m_penDraggingHandle = false;
        QPointF &handle = m_penNodes.last().handle;
        if (QPointF::dotProduct(handle, handle) < kMinPenHandleLength * kMinPenHandleLength) {
            handle = QPointF(0, 0);
        }
        m_documentItem->update();
        event->accept();
        return;
    }

    if (m_activeTool == Tool::Brush && m_brushDragging) {
        m_brushDragging = false;
        commitBrushStroke();
        event->accept();
        return;
    }

    if (m_dragging) {
        m_dragging = false;
        if (m_activeTool == Tool::Selection && !m_selection.isEmpty()) {
            const QPointF totalDelta = mapToScene(event->pos()) - m_dragStart;
            if (!totalDelta.isNull()) {
                for (engine::Shape *shape : m_selection) {
                    shape->translate(-totalDelta);
                }
                if (m_selection.size() == 1) {
                    m_document->undoStack()->push(new engine::TranslateShapeCommand(m_selection.first(), totalDelta));
                } else {
                    m_document->undoStack()->beginMacro(tr("Déplacer la sélection"));
                    for (engine::Shape *shape : m_selection) {
                        m_document->undoStack()->push(new engine::TranslateShapeCommand(shape, totalDelta));
                    }
                    m_document->undoStack()->endMacro();
                }
                if (m_recordingMacro) {
                    engine::MacroStep step;
                    step.kind = engine::MacroStep::Kind::Translate;
                    step.delta = totalDelta;
                    recordMacroStep(step);
                }
            }
        }
        emit statusMessage(tr("Prêt"));
    }

    m_documentItem->update();
    QGraphicsView::mouseReleaseEvent(event);
}

void CanvasView::mouseDoubleClickEvent(QMouseEvent *event) {
    if (m_activeTool == Tool::Pen && m_penNodes.size() >= 2) {
        auto shape = std::make_unique<engine::PathShape>(m_penNodes);
        m_penNodes.clear();
        m_penDraggingHandle = false;
        applyCurrentColor(shape.get());
        auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), tr("Tracé"));
        m_document->undoStack()->push(command);
        m_documentItem->update();
        emit statusMessage(tr("Tracé terminé"));
        event->accept();
        return;
    }
    QGraphicsView::mouseDoubleClickEvent(event);
}

void CanvasView::tabletEvent(QTabletEvent *event) {
    if (m_activeTool != Tool::Brush) {
        // Laisse Qt synthétiser un événement souris pour les autres outils.
        QGraphicsView::tabletEvent(event);
        return;
    }

    const QPointF scenePos = mapToScene(event->position().toPoint());
    const qreal pressure = std::max(event->pressure(), 0.05);

    switch (event->type()) {
        case QEvent::TabletPress:
            m_brushPoints.clear();
            m_brushPoints.append(engine::BrushPoint{scenePos, pressure});
            m_brushDragging = true;
            emit statusMessage(tr("Pinceau (tablette) : pression détectée"));
            break;
        case QEvent::TabletMove:
            if (m_brushDragging) {
                m_brushPoints.append(engine::BrushPoint{scenePos, pressure});
                viewport()->update();
            }
            break;
        case QEvent::TabletRelease:
            if (m_brushDragging) {
                m_brushDragging = false;
                commitBrushStroke();
            }
            break;
        default:
            break;
    }

    // Empêche Qt de synthétiser en plus un événement souris pour ce même
    // geste (on l'a déjà traité ici), ce qui dessinerait le trait deux fois.
    event->accept();
}

void CanvasView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape && !m_penNodes.isEmpty()) {
        m_penNodes.clear();
        m_penDraggingHandle = false;
        m_documentItem->update();
        emit statusMessage(tr("Tracé annulé"));
        event->accept();
        return;
    }

    if ((event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) && m_activeTool == Tool::Selection &&
        !m_selection.isEmpty()) {
        m_document->undoStack()->beginMacro(tr("Supprimer"));
        for (engine::Shape *shape : m_selection) {
            engine::Layer *owner = m_document->findLayerOf(shape);
            if (owner) {
                m_document->undoStack()->push(new engine::RemoveShapeCommand(owner, shape, tr("Supprimer")));
            }
        }
        m_document->undoStack()->endMacro();
        setSelection({});
        m_documentItem->update();
        emit statusMessage(tr("Forme(s) supprimée(s)"));
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Undo)) {
        m_document->undoStack()->undo();
        setSelection({});
        m_documentItem->update();
        emit statusMessage(tr("Annulé"));
        event->accept();
        return;
    }

    if (event->matches(QKeySequence::Redo)) {
        m_document->undoStack()->redo();
        setSelection({});
        m_documentItem->update();
        emit statusMessage(tr("Rétabli"));
        event->accept();
        return;
    }

    if (m_activeTool == Tool::Selection && (event->key() == Qt::Key_BracketRight || event->key() == Qt::Key_BracketLeft)) {
        const bool forward = event->key() == Qt::Key_BracketRight;
        const bool toExtreme = event->modifiers() & Qt::ShiftModifier;
        reorderSelection(forward, toExtreme);
        event->accept();
        return;
    }

    if (event->modifiers() == Qt::NoModifier) {
        switch (event->key()) {
            case Qt::Key_V: emit toolShortcutRequested(Tool::Selection); event->accept(); return;
            case Qt::Key_R: emit toolShortcutRequested(Tool::Rectangle); event->accept(); return;
            case Qt::Key_E: emit toolShortcutRequested(Tool::Ellipse); event->accept(); return;
            case Qt::Key_T: emit toolShortcutRequested(Tool::Text); event->accept(); return;
            case Qt::Key_P: emit toolShortcutRequested(Tool::Pen); event->accept(); return;
            case Qt::Key_B: emit toolShortcutRequested(Tool::Brush); event->accept(); return;
            default: break;
        }
    }

    QGraphicsView::keyPressEvent(event);
}

bool CanvasView::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_textEditor && event->type() == QEvent::KeyPress) {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Escape) {
            cancelTextEditor();
            return true;
        }
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) &&
            !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            commitTextEditor();
            return true;
        }
    }
    return QGraphicsView::eventFilter(watched, event);
}

void CanvasView::commitTextEditor() {
    if (!m_textEditor->isVisible()) {
        return;
    }
    const QString text = m_textEditor->toPlainText().trimmed();
    m_textEditor->hide();
    if (!text.isEmpty()) {
        auto shape = std::make_unique<engine::TextShape>(m_textEditorScenePos, text);
        if (m_colorExplicitlySet) {
            shape->fillColor = m_currentColor;
        }
        auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), tr("Texte"));
        m_document->undoStack()->push(command);
        m_documentItem->update();
    }
    setFocus();
}

void CanvasView::cancelTextEditor() {
    m_textEditor->hide();
    m_textEditor->clear();
    setFocus();
}

void CanvasView::commitBrushStroke() {
    if (m_brushPoints.size() < 2) {
        m_brushPoints.clear();
        return;
    }
    auto shape = std::make_unique<engine::BrushStroke>(m_brushPoints);
    m_brushPoints.clear();
    applyCurrentColor(shape.get());
    auto *command = new engine::AddShapeCommand(m_document->activeLayer(), std::move(shape), tr("Trait de pinceau"));
    m_document->undoStack()->push(command);
    m_documentItem->update();
    emit statusMessage(tr("Trait terminé"));
}

} // namespace agdraw::ui
