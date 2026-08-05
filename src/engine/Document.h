#pragma once

#include "Layer.h"

#include <memory>
#include <vector>

class QPainter;

namespace agdraw::engine {

// Modèle du document : indépendant de tout moteur de rendu (QPainter
// aujourd'hui, Skia demain). Source de vérité pour l'IU et l'export.
class Document {
public:
    Document();

    Layer &addLayer(const QString &name);
    Layer *activeLayer();
    void setActiveLayer(Layer *layer);

    const std::vector<std::unique_ptr<Layer>> &layers() const { return m_layers; }

    Shape *shapeAt(const QPointF &point) const;
    void paint(QPainter &painter) const;

private:
    std::vector<std::unique_ptr<Layer>> m_layers;
    Layer *m_activeLayer = nullptr;
};

} // namespace agdraw::engine
