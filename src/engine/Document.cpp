#include "Document.h"

namespace agdraw::engine {

Document::Document() {
    addLayer(QStringLiteral("Calque 1"));
}

Layer &Document::addLayer(const QString &name) {
    m_layers.push_back(std::make_unique<Layer>(name));
    m_activeLayer = m_layers.back().get();
    return *m_activeLayer;
}

Layer *Document::activeLayer() {
    return m_activeLayer;
}

void Document::setActiveLayer(Layer *layer) {
    m_activeLayer = layer;
}

void Document::clearLayers() {
    m_layers.clear();
    m_activeLayer = nullptr;
}

Shape *Document::shapeAt(const QPointF &point) const {
    for (auto layerIt = m_layers.rbegin(); layerIt != m_layers.rend(); ++layerIt) {
        Layer &layer = **layerIt;
        if (!layer.isVisible() || layer.isLocked()) {
            continue;
        }
        const auto &shapes = layer.shapes();
        for (auto shapeIt = shapes.rbegin(); shapeIt != shapes.rend(); ++shapeIt) {
            if ((*shapeIt)->contains(point)) {
                return shapeIt->get();
            }
        }
    }
    return nullptr;
}

Layer *Document::findLayerOf(Shape *shape) const {
    for (const auto &layer : m_layers) {
        if (layer->indexOf(shape) != layer->shapes().size()) {
            return layer.get();
        }
    }
    return nullptr;
}

void Document::paint(QPainter &painter) const {
    for (const auto &layer : m_layers) {
        layer->paint(painter);
    }
}

} // namespace agdraw::engine
