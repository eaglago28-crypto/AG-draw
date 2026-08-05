#include "PngExporter.h"

#include "Document.h"

#include <QImage>
#include <QPainter>

namespace agdraw::io {

bool exportPng(const engine::Document &document, const QString &path, const QRectF &sceneRect, const QSize &pixelSize,
               QString *errorMessage) {
    if (pixelSize.isEmpty() || sceneRect.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Dimensions d'export invalides.");
        }
        return false;
    }

    QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(pixelSize.width() / sceneRect.width(), pixelSize.height() / sceneRect.height());
    painter.translate(-sceneRect.topLeft());
    document.paint(painter);
    painter.end();

    if (!image.save(path, "PNG")) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Impossible d'enregistrer l'image PNG.");
        }
        return false;
    }
    return true;
}

} // namespace agdraw::io
