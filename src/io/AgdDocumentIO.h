#pragma once

#include <QString>

namespace agdraw::engine {
class Document;
}

namespace agdraw::io {

// Sauvegarde le document au format natif AGD (JSON).
bool saveAgd(const agdraw::engine::Document &document, const QString &path, QString *errorMessage = nullptr);

// Remplace le contenu de `document` par celui du fichier chargé (les
// calques existants sont effacés et la pile d'annulation est vidée).
// En cas d'échec, renvoie false, remplit `errorMessage` et laisse
// `document` inchangé.
bool loadAgd(agdraw::engine::Document &document, const QString &path, QString *errorMessage = nullptr);

} // namespace agdraw::io
