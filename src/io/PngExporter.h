#pragma once

#include <QRectF>
#include <QSize>
#include <QString>

namespace agdraw::engine {
class Document;
}

namespace agdraw::io {

// Exporte le document en PNG : rend `sceneRect` (coordonnées du document)
// dans une image de `pixelSize` pixels, fond blanc.
bool exportPng(const agdraw::engine::Document &document, const QString &path, const QRectF &sceneRect,
               const QSize &pixelSize, QString *errorMessage = nullptr);

} // namespace agdraw::io
