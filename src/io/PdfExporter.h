#pragma once

#include <QRectF>
#include <QString>

namespace agdraw::engine {
class Document;
}

namespace agdraw::io {

// Exporte `pageRect` (coordonnées du document) en PDF vectoriel (via
// QPdfWriter : les formes restent des tracés vectoriels dans le fichier,
// pas une image rastérisée). Si `includeCropMarks` est vrai, la page PDF
// est agrandie d'une marge de fond perdu et des repères d'impression
// (traits en L) sont dessinés aux 4 coins de la zone de rognage.
bool exportPdf(const agdraw::engine::Document &document, const QString &path, const QRectF &pageRect,
                bool includeCropMarks = false, QString *errorMessage = nullptr);

} // namespace agdraw::io
