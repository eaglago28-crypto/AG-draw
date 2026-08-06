#include "PdfExporter.h"

#include "Document.h"

#include <QPageLayout>
#include <QPageSize>
#include <QPainter>
#include <QPdfWriter>

namespace agdraw::io {

namespace {

constexpr qreal kBleedMargin = 20.0; // marge (unités document) autour de la page pour les repères.
constexpr qreal kMarkGap = 3.0;      // espace entre le bord de la page et le début du repère.
constexpr qreal kMarkLength = 14.0;  // longueur de chaque trait de repère.

void drawCropMarks(QPainter &painter, const QRectF &pageRect) {
    QPen pen(Qt::black, 0.5);
    painter.setPen(pen);

    const qreal left = pageRect.left();
    const qreal right = pageRect.right();
    const qreal top = pageRect.top();
    const qreal bottom = pageRect.bottom();

    // Chaque coin : un trait vertical et un trait horizontal, prolongeant
    // les bords de la page vers l'extérieur, séparés par un petit espace.
    for (const qreal x : {left, right}) {
        painter.drawLine(QPointF(x, top - kMarkGap), QPointF(x, top - kMarkGap - kMarkLength));
        painter.drawLine(QPointF(x, bottom + kMarkGap), QPointF(x, bottom + kMarkGap + kMarkLength));
    }
    for (const qreal y : {top, bottom}) {
        painter.drawLine(QPointF(left - kMarkGap, y), QPointF(left - kMarkGap - kMarkLength, y));
        painter.drawLine(QPointF(right + kMarkGap, y), QPointF(right + kMarkGap + kMarkLength, y));
    }
}

} // namespace

bool exportPdf(const engine::Document &document, const QString &path, const QRectF &pageRect, bool includeCropMarks,
               QString *errorMessage) {
    if (pageRect.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Dimensions de page invalides.");
        }
        return false;
    }

    const qreal margin = includeCropMarks ? kBleedMargin : 0.0;

    QPdfWriter writer(path);
    writer.setResolution(72);
    writer.setPageSize(QPageSize(QSizeF(pageRect.width() + margin * 2, pageRect.height() + margin * 2), QPageSize::Point));
    writer.setPageMargins(QMarginsF(0, 0, 0, 0), QPageLayout::Point);

    QPainter painter(&writer);
    if (!painter.isActive()) {
        if (errorMessage) {
            *errorMessage = QObject::tr("Impossible de créer le fichier PDF.");
        }
        return false;
    }
    painter.setRenderHint(QPainter::Antialiasing);
    painter.translate(margin, margin);
    painter.translate(-pageRect.topLeft());
    document.paint(painter);
    if (includeCropMarks) {
        drawCropMarks(painter, pageRect);
    }
    painter.end();

    return true;
}

} // namespace agdraw::io
