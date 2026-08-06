#pragma once

#include "Shape.h"

#include <QImage>
#include <QRectF>
#include <memory>
#include <vector>

namespace agdraw::engine {

struct TraceOptions {
    int threshold = 128;    // luminance (0-255) en dessous de laquelle un pixel est « encre ».
    int maxDimension = 220; // l'image est réduite (aspect conservé) avant traçage.
    int minBlobArea = 6;    // aire minimale (en pixels réduits) pour ignorer le bruit.
};

// Vectorise une image bitmap en une silhouette fermée par région connexe de
// pixels sombres (PathShape::closed = true, remplie de la couleur moyenne de
// la région). Les formes sont mises à l'échelle et centrées pour occuper
// `targetRect` en conservant les proportions de l'image d'origine.
//
// Limitations connues et volontaires pour cette première version :
// - Un seuil de luminance global sépare « encre » (sombre) et « fond »
//   (clair) : c'est un traceur de type logo/silhouette (comme un dessin au
//   trait sur fond clair), pas une segmentation multi-couleurs — une région
//   de couleur claire (luminance ≥ threshold) n'est pas détectée, même très
//   saturée.
// - Seul le contour extérieur de chaque région est conservé, les trous
//   internes (par exemple l'intérieur d'un « O ») ne sont pas préservés et
//   apparaissent remplis.
std::vector<std::unique_ptr<Shape>> traceBitmap(const QImage &image, const QRectF &targetRect,
                                                  const TraceOptions &options = {});

} // namespace agdraw::engine
