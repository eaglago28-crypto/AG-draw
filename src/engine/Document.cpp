#include "Document.h"

#include <algorithm>

namespace agdraw::engine {

Document::Document() {
    addLayer(QStringLiteral("Calque 1"));
    addPage(QStringLiteral("Page 1"), QRectF(0, 0, 794, 1123));
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

Page &Document::addPage(const QString &name, const QRectF &rect) {
    return *addPage(std::make_unique<Page>(name, rect));
}

Page *Document::addPage(std::unique_ptr<Page> page) {
    m_pages.push_back(std::move(page));
    m_activePage = m_pages.back().get();
    return m_activePage;
}

Page *Document::insertPage(size_t index, std::unique_ptr<Page> page) {
    index = std::min(index, m_pages.size());
    auto it = m_pages.insert(m_pages.begin() + static_cast<std::ptrdiff_t>(index), std::move(page));
    return it->get();
}

std::unique_ptr<Page> Document::takePage(Page *page) {
    auto it = std::find_if(m_pages.begin(), m_pages.end(),
                            [page](const std::unique_ptr<Page> &candidate) { return candidate.get() == page; });
    if (it == m_pages.end()) {
        return nullptr;
    }
    std::unique_ptr<Page> taken = std::move(*it);
    m_pages.erase(it);
    if (m_activePage == page) {
        m_activePage = m_pages.empty() ? nullptr : m_pages.front().get();
    }
    return taken;
}

size_t Document::indexOfPage(Page *page) const {
    for (size_t i = 0; i < m_pages.size(); ++i) {
        if (m_pages[i].get() == page) {
            return i;
        }
    }
    return m_pages.size();
}

Page *Document::activePage() {
    return m_activePage;
}

void Document::setActivePage(Page *page) {
    m_activePage = page;
}

void Document::clearPages() {
    m_pages.clear();
    m_activePage = nullptr;
}

void Document::removeMacro(const QString &name) {
    m_macros.erase(std::remove_if(m_macros.begin(), m_macros.end(),
                                   [&name](const Macro &macro) { return macro.name == name; }),
                   m_macros.end());
}

} // namespace agdraw::engine
