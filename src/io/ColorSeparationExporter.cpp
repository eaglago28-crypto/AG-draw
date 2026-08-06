#include "ColorSeparationExporter.h"

#include "Document.h"

#include <QImage>
#include <QPainter>
#include <algorithm>

namespace agdraw::io {

Cmyk rgbToCmyk(const QColor &color) {
    const qreal r = color.redF();
    const qreal g = color.greenF();
    const qreal b = color.blueF();

    const qreal k = 1.0 - std::max({r, g, b});
    if (k >= 1.0) {
        return Cmyk{0.0, 0.0, 0.0, 1.0};
    }
    Cmyk result;
    result.k = k;
    result.c = (1.0 - r - k) / (1.0 - k);
    result.m = (1.0 - g - k) / (1.0 - k);
    result.y = (1.0 - b - k) / (1.0 - k);
    return result;
}

namespace {

QImage renderPage(const engine::Document &document, const QRectF &sceneRect, const QSize &pixelSize) {
    QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(pixelSize.width() / sceneRect.width(), pixelSize.height() / sceneRect.height());
    painter.translate(-sceneRect.topLeft());
    document.paint(painter);
    painter.end();
    return image;
}

QImage extractSeparation(const QImage &rendered, qreal Cmyk::*channel) {
    QImage separation(rendered.size(), QImage::Format_Grayscale8);
    for (int y = 0; y < rendered.height(); ++y) {
        const QRgb *srcLine = reinterpret_cast<const QRgb *>(rendered.constScanLine(y));
        uchar *dstLine = separation.scanLine(y);
        for (int x = 0; x < rendered.width(); ++x) {
            const QColor pixelColor(srcLine[x]);
            const Cmyk cmyk = rgbToCmyk(pixelColor);
            const qreal ink = cmyk.*channel;
            dstLine[x] = static_cast<uchar>(255 - std::clamp(ink, 0.0, 1.0) * 255.0);
        }
    }
    return separation;
}

} // namespace

bool exportColorSeparations(const engine::Document &document, const QString &basePath, const QRectF &sceneRect,
                              const QSize &pixelSize, QString *errorMessage) {
    if (pixelSize.isEmpty() || sceneRect.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Dimensions d'export invalides.");
        }
        return false;
    }

    const QImage rendered = renderPage(document, sceneRect, pixelSize);

    const struct {
        QString suffix;
        qreal Cmyk::*channel;
    } channels[4] = {
        {QStringLiteral("_C.png"), &Cmyk::c},
        {QStringLiteral("_M.png"), &Cmyk::m},
        {QStringLiteral("_Y.png"), &Cmyk::y},
        {QStringLiteral("_K.png"), &Cmyk::k},
    };

    for (const auto &entry : channels) {
        const QImage separation = extractSeparation(rendered, entry.channel);
        if (!separation.save(basePath + entry.suffix, "PNG")) {
            if (errorMessage) {
                *errorMessage = QObject::tr("Impossible d'enregistrer la séparation %1.").arg(entry.suffix);
            }
            return false;
        }
    }
    return true;
}

} // namespace agdraw::io
