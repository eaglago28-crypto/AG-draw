#pragma once

#include <QColor>
#include <QRectF>
#include <QSize>
#include <QString>

namespace agdraw::engine {
class Document;
}

namespace agdraw::io {

// Quadrichromie CMJN naïve d'une couleur RGB (sans profil ICC — conversion
// « device » standard, comme la plupart des outils non gérés par un CMS).
// Chaque composante est dans [0, 1].
struct Cmyk {
    qreal c = 0.0;
    qreal m = 0.0;
    qreal y = 0.0;
    qreal k = 0.0;
};

Cmyk rgbToCmyk(const QColor &color);

// Exporte les 4 séparations couleur (Cyan/Magenta/Jaune/Noir) du document en
// niveaux de gris, un fichier PNG par plan d'encre : `basePath` + "_C.png",
// "_M.png", "_Y.png", "_K.png". Un pixel plus sombre indique plus d'encre
// sur ce plan (convention des films/épreuves de séparation traditionnels).
bool exportColorSeparations(const agdraw::engine::Document &document, const QString &basePath, const QRectF &sceneRect,
                              const QSize &pixelSize, QString *errorMessage = nullptr);

} // namespace agdraw::io
