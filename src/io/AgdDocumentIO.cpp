#include "AgdDocumentIO.h"

#include "BrushStroke.h"
#include "Document.h"
#include "EllipseShape.h"
#include "Layer.h"
#include "Page.h"
#include "PathShape.h"
#include "RectShape.h"
#include "TextShape.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUndoStack>

namespace agdraw::io {

namespace {

QJsonObject shapeCommonToJson(const engine::Shape &shape) {
    QJsonObject obj;
    obj["fill"] = shape.fillColor.name(QColor::HexArgb);
    obj["stroke"] = shape.strokeColor.name(QColor::HexArgb);
    obj["strokeWidth"] = shape.strokeWidth;

    obj["shadowEnabled"] = shape.shadowEnabled;
    obj["shadowColor"] = shape.shadowColor.name(QColor::HexArgb);
    obj["shadowOffsetX"] = shape.shadowOffset.x();
    obj["shadowOffsetY"] = shape.shadowOffset.y();

    obj["gradientEnabled"] = shape.gradientEnabled;
    obj["gradientStart"] = shape.gradientStartColor.name(QColor::HexArgb);
    obj["gradientEnd"] = shape.gradientEndColor.name(QColor::HexArgb);
    obj["gradientAngle"] = shape.gradientAngle;

    obj["contourEnabled"] = shape.contourEnabled;
    obj["contourSteps"] = shape.contourSteps;
    obj["contourOffset"] = shape.contourOffset;
    obj["contourColor"] = shape.contourColor.name(QColor::HexArgb);

    obj["envelopeEnabled"] = shape.envelopeEnabled;
    if (shape.envelopeCorners.size() == 4) {
        QJsonArray corners;
        for (const QPointF &corner : shape.envelopeCorners) {
            QJsonObject cornerObj;
            cornerObj["x"] = corner.x();
            cornerObj["y"] = corner.y();
            corners.append(cornerObj);
        }
        obj["envelopeCorners"] = corners;
    }

    obj["extrusionEnabled"] = shape.extrusionEnabled;
    obj["extrusionDepth"] = shape.extrusionDepth;
    obj["extrusionAngle"] = shape.extrusionAngle;
    obj["extrusionColor"] = shape.extrusionColor.name(QColor::HexArgb);

    return obj;
}

void applyShapeCommon(engine::Shape &shape, const QJsonObject &obj) {
    const QColor fill(obj.value("fill").toString());
    const QColor stroke(obj.value("stroke").toString());
    if (fill.isValid()) {
        shape.fillColor = fill;
    }
    if (stroke.isValid()) {
        shape.strokeColor = stroke;
    }
    shape.strokeWidth = obj.value("strokeWidth").toDouble(shape.strokeWidth);

    shape.shadowEnabled = obj.value("shadowEnabled").toBool(shape.shadowEnabled);
    const QColor shadowColor(obj.value("shadowColor").toString());
    if (shadowColor.isValid()) {
        shape.shadowColor = shadowColor;
    }
    shape.shadowOffset = QPointF(obj.value("shadowOffsetX").toDouble(shape.shadowOffset.x()),
                                  obj.value("shadowOffsetY").toDouble(shape.shadowOffset.y()));

    shape.gradientEnabled = obj.value("gradientEnabled").toBool(shape.gradientEnabled);
    const QColor gradientStart(obj.value("gradientStart").toString());
    if (gradientStart.isValid()) {
        shape.gradientStartColor = gradientStart;
    }
    const QColor gradientEnd(obj.value("gradientEnd").toString());
    if (gradientEnd.isValid()) {
        shape.gradientEndColor = gradientEnd;
    }
    shape.gradientAngle = obj.value("gradientAngle").toDouble(shape.gradientAngle);

    shape.contourEnabled = obj.value("contourEnabled").toBool(shape.contourEnabled);
    shape.contourSteps = obj.value("contourSteps").toInt(shape.contourSteps);
    shape.contourOffset = obj.value("contourOffset").toDouble(shape.contourOffset);
    const QColor contourColor(obj.value("contourColor").toString());
    if (contourColor.isValid()) {
        shape.contourColor = contourColor;
    }

    shape.envelopeEnabled = obj.value("envelopeEnabled").toBool(shape.envelopeEnabled);
    const QJsonArray cornersArray = obj.value("envelopeCorners").toArray();
    if (cornersArray.size() == 4) {
        QVector<QPointF> corners;
        for (const QJsonValue &value : cornersArray) {
            const QJsonObject cornerObj = value.toObject();
            corners.append(QPointF(cornerObj.value("x").toDouble(), cornerObj.value("y").toDouble()));
        }
        shape.envelopeCorners = corners;
    }

    shape.extrusionEnabled = obj.value("extrusionEnabled").toBool(shape.extrusionEnabled);
    shape.extrusionDepth = obj.value("extrusionDepth").toDouble(shape.extrusionDepth);
    shape.extrusionAngle = obj.value("extrusionAngle").toDouble(shape.extrusionAngle);
    const QColor extrusionColor(obj.value("extrusionColor").toString());
    if (extrusionColor.isValid()) {
        shape.extrusionColor = extrusionColor;
    }
}

QJsonObject rectToJson(const engine::RectShape &shape) {
    QJsonObject obj = shapeCommonToJson(shape);
    obj["type"] = "rect";
    obj["x"] = shape.rect.x();
    obj["y"] = shape.rect.y();
    obj["w"] = shape.rect.width();
    obj["h"] = shape.rect.height();
    return obj;
}

QJsonObject ellipseToJson(const engine::EllipseShape &shape) {
    QJsonObject obj = shapeCommonToJson(shape);
    obj["type"] = "ellipse";
    obj["x"] = shape.rect.x();
    obj["y"] = shape.rect.y();
    obj["w"] = shape.rect.width();
    obj["h"] = shape.rect.height();
    return obj;
}

QJsonObject pathToJson(const engine::PathShape &shape) {
    QJsonObject obj = shapeCommonToJson(shape);
    obj["type"] = "path";
    QJsonArray nodes;
    for (const engine::PathNode &node : shape.nodes) {
        QJsonObject nodeObj;
        nodeObj["x"] = node.point.x();
        nodeObj["y"] = node.point.y();
        nodeObj["hx"] = node.handle.x();
        nodeObj["hy"] = node.handle.y();
        nodes.append(nodeObj);
    }
    obj["nodes"] = nodes;
    return obj;
}

QJsonObject textToJson(const engine::TextShape &shape) {
    QJsonObject obj = shapeCommonToJson(shape);
    obj["type"] = "text";
    obj["x"] = shape.position.x();
    obj["y"] = shape.position.y();
    obj["text"] = shape.text;
    obj["fontSize"] = shape.fontPointSize;
    return obj;
}

QJsonObject brushToJson(const engine::BrushStroke &shape) {
    QJsonObject obj = shapeCommonToJson(shape);
    obj["type"] = "brush";
    obj["baseWidth"] = shape.baseWidth;
    QJsonArray points;
    for (const engine::BrushPoint &point : shape.points) {
        QJsonObject pointObj;
        pointObj["x"] = point.point.x();
        pointObj["y"] = point.point.y();
        pointObj["pressure"] = point.pressure;
        points.append(pointObj);
    }
    obj["points"] = points;
    return obj;
}

QJsonObject shapeToJson(const engine::Shape &shape) {
    if (const auto *rect = dynamic_cast<const engine::RectShape *>(&shape)) {
        return rectToJson(*rect);
    }
    if (const auto *ellipse = dynamic_cast<const engine::EllipseShape *>(&shape)) {
        return ellipseToJson(*ellipse);
    }
    if (const auto *path = dynamic_cast<const engine::PathShape *>(&shape)) {
        return pathToJson(*path);
    }
    if (const auto *text = dynamic_cast<const engine::TextShape *>(&shape)) {
        return textToJson(*text);
    }
    if (const auto *brush = dynamic_cast<const engine::BrushStroke *>(&shape)) {
        return brushToJson(*brush);
    }
    return {};
}

std::unique_ptr<engine::Shape> shapeFromJson(const QJsonObject &obj) {
    const QString type = obj.value("type").toString();
    std::unique_ptr<engine::Shape> shape;

    if (type == QLatin1String("rect")) {
        shape = std::make_unique<engine::RectShape>(
            QRectF(obj.value("x").toDouble(), obj.value("y").toDouble(), obj.value("w").toDouble(), obj.value("h").toDouble()));
    } else if (type == QLatin1String("ellipse")) {
        shape = std::make_unique<engine::EllipseShape>(
            QRectF(obj.value("x").toDouble(), obj.value("y").toDouble(), obj.value("w").toDouble(), obj.value("h").toDouble()));
    } else if (type == QLatin1String("path")) {
        QVector<engine::PathNode> nodes;
        for (const QJsonValue &value : obj.value("nodes").toArray()) {
            const QJsonObject nodeObj = value.toObject();
            nodes.append(engine::PathNode{QPointF(nodeObj.value("x").toDouble(), nodeObj.value("y").toDouble()),
                                           QPointF(nodeObj.value("hx").toDouble(), nodeObj.value("hy").toDouble())});
        }
        shape = std::make_unique<engine::PathShape>(nodes);
    } else if (type == QLatin1String("text")) {
        shape = std::make_unique<engine::TextShape>(QPointF(obj.value("x").toDouble(), obj.value("y").toDouble()),
                                                      obj.value("text").toString());
        static_cast<engine::TextShape *>(shape.get())->fontPointSize = obj.value("fontSize").toDouble(24.0);
    } else if (type == QLatin1String("brush")) {
        QVector<engine::BrushPoint> points;
        for (const QJsonValue &value : obj.value("points").toArray()) {
            const QJsonObject pointObj = value.toObject();
            points.append(engine::BrushPoint{QPointF(pointObj.value("x").toDouble(), pointObj.value("y").toDouble()),
                                              pointObj.value("pressure").toDouble(1.0)});
        }
        shape = std::make_unique<engine::BrushStroke>(points, obj.value("baseWidth").toDouble(8.0));
    }

    if (shape) {
        applyShapeCommon(*shape, obj);
    }
    return shape;
}

} // namespace

bool saveAgd(const engine::Document &document, const QString &path, QString *errorMessage) {
    QJsonArray layersArray;
    for (const auto &layer : document.layers()) {
        QJsonObject layerObj;
        layerObj["name"] = layer->name();
        layerObj["visible"] = layer->isVisible();
        layerObj["locked"] = layer->isLocked();

        QJsonArray shapesArray;
        for (const auto &shape : layer->shapes()) {
            shapesArray.append(shapeToJson(*shape));
        }
        layerObj["shapes"] = shapesArray;
        layersArray.append(layerObj);
    }

    QJsonArray pagesArray;
    for (const auto &page : document.pages()) {
        QJsonObject pageObj;
        pageObj["name"] = page->name();
        pageObj["x"] = page->rect().x();
        pageObj["y"] = page->rect().y();
        pageObj["w"] = page->rect().width();
        pageObj["h"] = page->rect().height();
        pagesArray.append(pageObj);
    }

    QJsonObject root;
    root["format"] = "AG Draw";
    root["version"] = 1;
    root["layers"] = layersArray;
    root["pages"] = pagesArray;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Impossible d'écrire le fichier : %1").arg(file.errorString());
        }
        return false;
    }
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

bool loadAgd(engine::Document &document, const QString &path, QString *errorMessage) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Impossible d'ouvrir le fichier : %1").arg(file.errorString());
        }
        return false;
    }

    QJsonParseError parseError{};
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !jsonDoc.isObject()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Fichier AGD invalide : %1").arg(parseError.errorString());
        }
        return false;
    }

    const QJsonObject root = jsonDoc.object();
    const QJsonArray layersArray = root.value("layers").toArray();

    document.clearLayers();
    for (const QJsonValue &layerValue : layersArray) {
        const QJsonObject layerObj = layerValue.toObject();
        engine::Layer &layer = document.addLayer(layerObj.value("name").toString(QObject::tr("Calque")));
        layer.setVisible(layerObj.value("visible").toBool(true));
        layer.setLocked(layerObj.value("locked").toBool(false));
        for (const QJsonValue &shapeValue : layerObj.value("shapes").toArray()) {
            if (auto shape = shapeFromJson(shapeValue.toObject())) {
                layer.addShape(std::move(shape));
            }
        }
    }
    if (document.layers().empty()) {
        document.addLayer(QObject::tr("Calque 1"));
    }

    document.clearPages();
    const QJsonArray pagesArray = root.value("pages").toArray();
    for (const QJsonValue &pageValue : pagesArray) {
        const QJsonObject pageObj = pageValue.toObject();
        document.addPage(pageObj.value("name").toString(QObject::tr("Page")),
                          QRectF(pageObj.value("x").toDouble(), pageObj.value("y").toDouble(),
                                 pageObj.value("w").toDouble(794), pageObj.value("h").toDouble(1123)));
    }
    if (document.pages().empty()) {
        // Fichier sans page (ancien format ou fichier malformé) : page de
        // secours pour garder le document utilisable.
        document.addPage(QObject::tr("Page 1"), QRectF(0, 0, 794, 1123));
    }

    document.undoStack()->clear();
    return true;
}

} // namespace agdraw::io
