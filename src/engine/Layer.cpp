#include "Layer.h"

#include <algorithm>

namespace agdraw::engine {

Shape *Layer::addShape(std::unique_ptr<Shape> shape) {
    m_shapes.push_back(std::move(shape));
    return m_shapes.back().get();
}

void Layer::removeShape(Shape *shape) {
    m_shapes.erase(
        std::remove_if(m_shapes.begin(), m_shapes.end(),
                        [shape](const std::unique_ptr<Shape> &candidate) { return candidate.get() == shape; }),
        m_shapes.end());
}

std::unique_ptr<Shape> Layer::takeShape(Shape *shape) {
    auto it = std::find_if(m_shapes.begin(), m_shapes.end(),
                            [shape](const std::unique_ptr<Shape> &candidate) { return candidate.get() == shape; });
    if (it == m_shapes.end()) {
        return nullptr;
    }
    std::unique_ptr<Shape> taken = std::move(*it);
    m_shapes.erase(it);
    return taken;
}

void Layer::paint(QPainter &painter) const {
    if (!m_visible) {
        return;
    }
    for (const auto &shape : m_shapes) {
        shape->paint(painter);
    }
}

} // namespace agdraw::engine
