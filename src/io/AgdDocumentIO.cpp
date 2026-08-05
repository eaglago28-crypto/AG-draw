#include "AgdDocumentIO.h"

#include "Document.h"
#include "EllipseShape.h"
#include "Layer.h"
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

    QJsonObject root;
    root["format"] = "AG Draw";
    root["version"] = 1;
    root["layers"] = layersArray;

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
    document.undoStack()->clear();
    return true;
}

} // namespace agdraw::io
